import sys
import typing as t
import logging

import grpc
from flask import Flask, jsonify, request
from concurrent.futures import ThreadPoolExecutor

from rpc import raft_pb2_grpc, raft_pb2
from raft_node import RaftNode
import nodes

app: Flask = Flask(__name__)
raft_node: RaftNode | None = None


# Disable Werkzeug logs
log = logging.getLogger("werkzeug")
log.disabled = True


def get_raft_node() -> RaftNode:
    if raft_node is None:
        raise RuntimeError("Server wasn't started")

    return raft_node


@app.route("/kv_storage/<key>", methods=["GET"])
def read(key):
    stage: str = request.args["stage"]
    if stage == "initial":
        status, json = get_raft_node().replicate_entry(
            raft_pb2.LogEntry(read=raft_pb2.ReadOperation(key=key))
        )
    else:
        operation_id: int = int(request.args["operation_id"])
        status, json = get_raft_node().state_machine.get_operation_result(operation_id)
    return jsonify(json), status


@app.route("/kv_storage", methods=["POST"])
def create():
    key: str = request.get_json()["key"]
    value: str = request.get_json()["value"]
    status, json = get_raft_node().replicate_entry(
        raft_pb2.LogEntry(create=raft_pb2.CreateOperation(key=key, value=value))
    )
    return jsonify(json), status


@app.route("/kv_storage", methods=["PUT"])
def update():
    key: str = request.get_json()["key"]
    value: str = request.get_json()["value"]
    status, json = get_raft_node().replicate_entry(
        raft_pb2.LogEntry(update=raft_pb2.UpdateOperation(key=key, value=value))
    )
    return jsonify(json), status


@app.route("/kv_storage", methods=["PATCH"])
def cas():
    key: str = request.get_json()["key"]
    expected_value: str = request.get_json()["expected_value"]
    new_value: str = request.get_json()["new_value"]
    status, json = get_raft_node().replicate_entry(
        raft_pb2.LogEntry(
            cas=raft_pb2.CASOperation(
                key=key, expectedValue=expected_value, newValue=new_value
            )
        )
    )
    return jsonify(json), status


@app.route("/kv_storage/<key>", methods=["DELETE"])
def delete(key):
    status, json = get_raft_node().replicate_entry(
        raft_pb2.LogEntry(delete=raft_pb2.DeleteOperation(key=key))
    )
    return jsonify(json), status


def start_raft_node(node_id: int) -> t.Tuple[RaftNode, grpc.Server]:
    server: grpc.Server = grpc.server(ThreadPoolExecutor(max_workers=10))
    raft_node: RaftNode = RaftNode(node_id)
    raft_pb2_grpc.add_RaftServicer_to_server(raft_node, server)
    port: int = nodes.grpc_ports[node_id]
    server.add_insecure_port(nodes.GetGRPCAddress(node_id))
    server.start()
    print(f"Raft node {node_id} started on port {port}")
    return raft_node, server


if __name__ == "__main__":
    node_id: int = int(sys.argv[1])
    assert node_id < nodes.nodes_cnt
    raft_node, server = start_raft_node(node_id)
    app.run(port=nodes.http_ports[node_id], threaded=True)
