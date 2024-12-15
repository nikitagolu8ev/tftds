import unittest
import subprocess
import time
import threading
import os
import signal

from client import Client
import nodes


class Tests(unittest.TestCase):
    def start_servers(self):
        servers = [self.start_server(i) for i in range(nodes.nodes_cnt)]
        return servers

    def start_server(self, server_id):
        server = subprocess.Popen(["python", "server.py", str(server_id)])
        return server

    def stop_servers(self, servers):
        for server in servers:
            self.stop_server(server)

    def stop_server(self, server):
        os.kill(server.pid, signal.SIGKILL)
        server.wait()

    def test_simple(self):
        client = Client()
        servers = self.start_servers()
        self.assertTrue(client.create(key="key_1", value="value_1"))
        self.assertEqual(client.read(key="key_1"), "value_1")
        self.assertIsNone(client.read(key="key_2"))

        self.assertTrue(client.update(key="key_1", value="value_2"))
        self.assertEqual(client.read(key="key_1"), "value_2")

        self.assertFalse(
            client.cas(key="key_1", expected_value="value_1", new_value="value_2")
        )
        self.assertTrue(
            client.cas(key="key_1", expected_value="value_2", new_value="value_3")
        )
        self.assertEqual(client.read(key="key_1"), "value_3")

        self.assertTrue(client.delete(key="key_1"))
        self.assertFalse(client.delete(key="key_1"))
        self.assertIsNone(client.read("key_1"))

        self.stop_servers(servers)

    def test_fault_recovery(self):
        client = Client()
        servers = self.start_servers()
        self.assertTrue(client.create(key="key_1", value="value_1"))

        for i in range(nodes.majority_cnt):
            self.stop_server(servers[i])

        has_been_read = []

        def read_thread_target(has_been_read):
            self.assertEqual(client.read(key="key_1"), "value_1")
            has_been_read.append(True)

        thread = threading.Thread(target=read_thread_target, args=(has_been_read,))
        thread.start()

        time.sleep(1)

        self.assertListEqual(has_been_read, [])
        servers[0] = self.start_server(0)
        
        time.sleep(1)

        self.assertListEqual(has_been_read, [True])
        self.stop_servers([servers[0]] + servers[nodes.majority_cnt:])

    def test_suspend_server(self):
        client = Client()
        servers = self.start_servers()
        self.assertTrue(client.create(key="key_1", value="value_1"))

        for i in range(nodes.majority_cnt):
            os.kill(servers[i].pid, signal.SIGSTOP)

        has_been_read = []

        def read_thread_target(has_been_read):
            self.assertEqual(client.read(key="key_1"), "value_1")
            has_been_read.append(True)

        thread = threading.Thread(target=read_thread_target, args=(has_been_read,))
        thread.start()

        time.sleep(1)

        self.assertListEqual(has_been_read, [])
        os.kill(servers[0].pid, signal.SIGCONT)
        
        time.sleep(1)

        self.assertListEqual(has_been_read, [True])
        self.stop_servers([servers[0]] + servers[nodes.majority_cnt:])


if __name__ == "__main__":
    unittest.main()
