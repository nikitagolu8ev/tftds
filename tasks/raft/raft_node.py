import threading
from enum import Enum
import random
from concurrent.futures import ThreadPoolExecutor, as_completed
import sys

import grpc

from rpc import raft_pb2, raft_pb2_grpc
import nodes
from state_machine import StateMachine

class Role(Enum):
    FOLLOWER = 0
    CANDIDATE = 1
    LEADER = 2

class RaftNode(raft_pb2_grpc.RaftServicer):
    def __init__(self, node_id):
        self.node_id = node_id
        self.node_addresses = nodes.GetAddresses()
        self.current_leader = -1

        self.state_machine = StateMachine()

        self.current_term = 0
        self.voted_for = None
        self.log = [raft_pb2.LogEntry(term=0, init=raft_pb2.InitOperation())]

        self.commit_index = 0
        self.last_applied = 0

        self.next_index = [1] * nodes.nodes_cnt
        self.match_index = [0] * nodes.nodes_cnt
        
        self.role = Role.FOLLOWER

        self.election_timer = None
        self.heartbeat_timer = None

        self.lock = threading.Condition()

        self.start()

    def start(self):
        self.reset_election_timer()

    # unlocked
    def reset_heartbeat_timer(self):
        if self.heartbeat_timer:
            self.heartbeat_timer.cancel()

        self.heartbeat_timer = threading.Timer(0.05, self.piggyback_heartbeat)
        self.heartbeat_timer.start()

    # unlocked
    def schedule_heartbeat(self):
        if self.heartbeat_timer:
            self.heartbeat_timer.cancel()

        self.heartbeat_timer = threading.Timer(0, self.piggyback_heartbeat)
        self.heartbeat_timer.start()

    # locked
    def piggyback_heartbeat(self):
        threads = []

        with self.lock:
            self_node_id = self.node_id

        for i in range(nodes.nodes_cnt):
            if i != self_node_id:
                threads.append(threading.Thread(target=self.append_entries, args=(i,)))
                threads[-1].start()

        for thread in threads:
            thread.join()

        # check that leader wasn't changed
        with self.lock:
            if self.role == Role.LEADER:
                self.reset_heartbeat_timer()

    # unlocked
    def reset_election_timer(self):
        if self.election_timer:
            self.election_timer.cancel()

        timer_duration = random.uniform(0.15, 0.3)
        self.election_timer = threading.Timer(timer_duration, self.start_election)
        self.election_timer.start()

    # locked
    def start_election(self):
        with self.lock:
            print("Became a candidate")
            self.role = Role.CANDIDATE
            self.current_term += 1
            self.voted_for = self.node_id
            self_node_id = self.voted_for

        votes = 1

        with ThreadPoolExecutor(max_workers=nodes.nodes_cnt - 1) as executor:
            futures = [executor.submit(self.request_vote, node_id) for node_id in range(nodes.nodes_cnt) if node_id != self_node_id]

            for future in futures:
                try:
                    if future.result():
                        votes += 1
                except grpc.RpcError as e:
                    pass
                    # print(f"Failed to connect with node")

        with self.lock:
            if self.role == Role.CANDIDATE and votes >= nodes.majority_cnt:
                self.become_leader()
            else:
                self.reset_election_timer()

    # unlocked
    def become_leader(self):
        print("Promoted to leader")
        if self.election_timer:
            self.election_timer.cancel()

        self.role = Role.LEADER
        self.next_index = [len(self.log)] * nodes.nodes_cnt
        self.match_index = [0] * nodes.nodes_cnt
        self.match_index[self.node_id] = len(self.log) - 1
        self.schedule_heartbeat()

    # unlocked
    def downgrade_to_follower(self, term):
        print("Downgraded to follower")
        if self.heartbeat_timer:
            self.heartbeat_timer.cancel()

        self.current_term = term
        self.voted_for = None
        self.role = Role.FOLLOWER
        self.reset_election_timer()

    # unlocked
    def get_match_index_median(self):
        return sorted(self.match_index)[nodes.nodes_cnt // 2]

    # locked
    def append_entries(self, node_id):
        with self.lock:
            if self.role != Role.LEADER:
                return
            address = self.node_addresses[node_id]
            term = self.current_term
            leader_id = self.node_id
            prev_log_index = self.next_index[node_id] - 1
            # print(f"prev log index of node {node_id} is {prev_log_index}")
            prev_log_term = self.log[prev_log_index].term
            entries = self.log[self.next_index[node_id]:]
            leader_commit = self.commit_index
            # print(term, leader_id, prev_log_index, prev_log_term, leader_commit)
            request = raft_pb2.AppendEntriesRequest(
                term=term,
                leaderId=leader_id,
                prevLogIndex=prev_log_index,
                prevLogTerm=prev_log_term,
                entries=entries,
                leaderCommit=leader_commit,
            )

        with grpc.insecure_channel(address) as channel:
            stub = raft_pb2_grpc.RaftStub(channel)
            try:
                response = stub.AppendEntries(request, timeout=0.05)
                with self.lock:
                    if response.term > self.current_term:
                        self.downgrade_to_follower(self.current_term)
                        return

                    if not response.success: 
                        self.next_index[node_id] -= 1
                        # if self.next_index[node_id] == 0:
                            # print("NEGATIVE PREFIX, NEGATIVE PREFIX\n" * 30)
                    else:
                        self.match_index[node_id] = request.prevLogIndex + len(request.entries)
                        # print(f"match index of node {node_id} is {self.match_index[node_id]}, request entries is {len(request.entries)}")
                        self.next_index[node_id] = self.match_index[node_id] + 1
                        median = self.get_match_index_median()

                        if self.log[median].term == self.current_term and self.commit_index < median:
                            self.state_machine.process_new_entries(self.log[self.commit_index+1:median+1])
                            self.commit_index = median
                            self.lock.notify_all()

            except grpc.RpcError as e:
                pass
                # print(f"Failed to connect with node {node_id}: {e}")

    # locked
    def request_vote(self, node_id):
        with self.lock:
            address = self.node_addresses[node_id]
            request = raft_pb2.RequestVoteRequest(
                term=self.current_term,
                candidateId=self.node_id,
                lastLogIndex=len(self.log) - 1,
                lastLogTerm=self.log[-1].term,
            )

        with grpc.insecure_channel(address) as channel:
            stub = raft_pb2_grpc.RaftStub(channel)

            response = stub.RequestVote(request, timeout=0.05)
            with self.lock:
                if response.term > self.current_term:
                    self.downgrade_to_follower(self.current_term)
                
                return response.voteGranted

    # locked
    def replicate_entry(self, entry):
        with self.lock:
            if self.role != Role.LEADER:
                return False, self.current_leader

            entry.term = self.current_term
            self.log.append(entry)
            self.match_index[self.node_id] = len(self.log) - 1
            entry_id = len(self.log) - 1
            self.schedule_heartbeat()

            while self.commit_index < entry_id or self.role != Role.LEADER:
                self.lock.wait()

            if self.role != Role.LEADER:
                return False, self.current_leader

            return self.state_machine.get_operation_result(entry_id), self.current_leader

    # locked
    def test_log(self):
        with self.lock:
            return self.log

    # locked
    def AppendEntries(self, request, context):
        with self.lock:
            response = raft_pb2.AppendEntriesResponse(term=self.current_term, success=False)
            if self.current_term > request.term:
                return response
            
            if self.role != Role.FOLLOWER:
                self.downgrade_to_follower(request.term)

            self.current_term = request.term
            self.current_leader = request.leaderId
            self.reset_election_timer()

            if request.prevLogIndex >= len(self.log):
                print(f"My log is shorter than {request.prevLogIndex}")
                return response

            if self.log[request.prevLogIndex].term != request.prevLogTerm:
                self.log[request.prevLogIndex:] = []
                print(f"My log have different prefix at position {request.prevLogIndex}")
                return response

            response.success = True
            for i in range(len(request.entries)):
                log_id = request.prevLogIndex + 1 + i
                if log_id >= len(self.log) or self.log[log_id].term != request.entries[i]:
                    self.log[log_id:] = request.entries[i:]

            if self.commit_index < request.leaderCommit:
                new_commit_index = min(request.leaderCommit, request.prevLogIndex + len(request.entries))
                self.state_machine.process_new_entries(self.log[self.commit_index+1:new_commit_index+1])
                self.commit_index = min(request.leaderCommit, request.prevLogIndex + len(request.entries))
                self.lock.notify_all()

            return response

    # locked
    def RequestVote(self, request, context):
        with self.lock:
            response = raft_pb2.RequestVoteResponse(term=self.current_term, voteGranted=False)

            if request.term < self.current_term:
                return response

            if request.term > self.current_term:
                self.downgrade_to_follower(request.term)

            if self.voted_for is None or self.voted_for == request.candidateId and (
                self.log[-1].term < request.lastLogTerm or                                               # check that log
                (self.log[-1].term == request.lastLogTerm and len(self.log) - 1 <= request.lastLogIndex)  # is not stale
            ):
                self.voted_for = request.candidateId
                response.voteGranted = True
                self.reset_election_timer()

            return response

def start_server(node_id):
    server = grpc.server(ThreadPoolExecutor(max_workers=10))
    raft_node = RaftNode(node_id)
    raft_pb2_grpc.add_RaftServicer_to_server(raft_node, server)
    port = nodes.ports[node_id]
    server.add_insecure_port(nodes.GetAddress(node_id))
    server.start()
    print(f"Raft node {node_id} started on port {port}")
    return raft_node, server

if __name__ == "__main__":
    node_id = int(sys.argv[1])
    assert(node_id < nodes.nodes_cnt)
    raft_node, server = start_server(node_id)

    while True:
        print("Enter operation type:")
        operation_type = input()
        log_entry = None
        if operation_type == "CREATE":
            print("Enter key and value to create:")
            key, value = input().split()
            log_entry = raft_pb2.LogEntry(term=0, create=raft_pb2.CreateOperation(key=key, value=value))
        elif operation_type == "READ":
            print("Enter key to read:")
            key = input()
            log_entry = raft_pb2.LogEntry(term=0, read=raft_pb2.ReadOperation(key=key))
        elif operation_type == "UPDATE":
            print("Enter key and value to update:")
            key, value = input().split()
            log_entry = raft_pb2.LogEntry(term=0, update=raft_pb2.UpdateOperation(key=key, value=value))
        elif operation_type == "CAS":
            print("Enter key, expected value and new value to cas:")
            key, expected_value, new_value = input().split()
            log_entry = raft_pb2.LogEntry(term=0, cas=raft_pb2.CASOperation(
                key=key,
                expectedValue=expected_value,
                newValue=new_value
            ))
        elif operation_type == "DELETE":
            print("Enter key to delete:")
            key = input()
            log_entry = raft_pb2.LogEntry(term=0, delete=raft_pb2.DeleteOperation(key=key))
        elif operation_type == "LOG":
            print(raft_node.test_log())
            continue
        else:
            print(f"Unsopported operation: {operation_type}")
            continue

        result = raft_node.replicate_entry(log_entry)
        print(result)

