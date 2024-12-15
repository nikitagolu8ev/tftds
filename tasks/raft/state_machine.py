import typing as t
from http import HTTPStatus
from threading import Condition

from rpc import raft_pb2


class StateMachine:
    def __init__(self):
        self.kv_storage: t.Dict[str, str] = {}
        self.log_entry_op_result: t.List[t.Tuple[int, t.Dict[str, str]]] = []
        self.handlers: t.Dict[str, t.Callable] = {
            "create": self.handle_create,
            "read": self.handle_read,
            "update": self.handle_update,
            "cas": self.handle_cas,
            "delete": self.handle_delete,
        }
        self.lock = Condition()

    def handle_create(self, operation_data: raft_pb2.CreateOperation) -> None:
        if operation_data.key in self.kv_storage:
            self.log_entry_op_result.append(
                (HTTPStatus.BAD_REQUEST, {"error": "key already exists"})
            )
        else:
            self.kv_storage[operation_data.key] = operation_data.value
            self.log_entry_op_result.append((HTTPStatus.OK, {"status": "successful"}))

    def handle_read(self, operation_data: raft_pb2.ReadOperation) -> None:
        if operation_data.key in self.kv_storage:
            self.log_entry_op_result.append(
                (HTTPStatus.OK, {"value": self.kv_storage[operation_data.key]})
            )
        else:
            self.log_entry_op_result.append(
                (HTTPStatus.NOT_FOUND, {"error": "no value with such key"})
            )

    def handle_update(self, operation_data: raft_pb2.UpdateOperation) -> None:
        if operation_data.key in self.kv_storage:
            self.kv_storage[operation_data.key] = operation_data.value
            self.log_entry_op_result.append((HTTPStatus.OK, {"status": "successful"}))
        else:
            self.log_entry_op_result.append(
                (HTTPStatus.NOT_FOUND, {"error": "no value with such key"})
            )

    def handle_cas(self, operation_data: raft_pb2.CASOperation) -> None:
        if operation_data.key in self.kv_storage:
            if self.kv_storage[operation_data.key] != operation_data.expectedValue:
                self.log_entry_op_result.append(
                    (
                        HTTPStatus.BAD_REQUEST,
                        {"error": "value in storage doesn't match with expected value"},
                    )
                )
            else:
                self.kv_storage[operation_data.key] = operation_data.newValue
                self.log_entry_op_result.append(
                    (HTTPStatus.OK, {"status": "successful"})
                )
        else:
            self.log_entry_op_result.append(
                (HTTPStatus.NOT_FOUND, {"error": "no value with such key"})
            )

    def handle_delete(self, operation_data: raft_pb2.DeleteOperation) -> None:
        if operation_data.key in self.kv_storage:
            del self.kv_storage[operation_data.key]
            self.log_entry_op_result.append((HTTPStatus.OK, {"status": "successful"}))
        else:
            self.log_entry_op_result.append(
                (HTTPStatus.NOT_FOUND, {"error": "no value with such key"})
            )

    def process_new_entries(self, entries: t.List[raft_pb2.LogEntry]):
        with self.lock:
            for entry in entries:
                # print(f"new entry processed:\n{entry}")
                operation_type = entry.WhichOneof("operation")
                operation_data = getattr(entry, operation_type)

                handler = self.handlers[operation_type]
                handler(operation_data)
            self.lock.notify_all()

    def get_operation_result(self, entry_id) -> t.Tuple[int, t.Dict[str, str]]:
        # init operation isn't commited, so subtract 1
        with self.lock:
            while entry_id > len(self.log_entry_op_result):
                self.lock.wait()
            return self.log_entry_op_result[entry_id - 1]
