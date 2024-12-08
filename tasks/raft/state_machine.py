class StateMachine:
    def __init__(self):
        self.kv_storage = {}
        self.log_entry_op_result = []
        self.handlers = {
            "init": self.handle_init,
            "create": self.handle_create,
            "read": self.handle_read,
            "update": self.handle_update,
            "cas": self.handle_cas,
            "delete": self.handle_delete,
        }

    def handle_init(self, operation_data):
        self.log_entry_op_result.append((True, "init"))

    def handle_create(self, operation_data):
        if operation_data.key in self.kv_storage:
            self.log_entry_op_result.append((False, "key already exists"))
        else:
            self.kv_storage[operation_data.key] = operation_data.value
            self.log_entry_op_result.append((True, "success"))

    def handle_read(self, operation_data):
        if operation_data.key in self.kv_storage:
            self.log_entry_op_result.append((True, self.kv_storage[operation_data.key]))
        else:
            self.log_entry_op_result.append((False, "no value with such key"))

    def handle_update(self, operation_data):
        if operation_data.key in self.kv_storage:
            self.kv_storage[operation_data.key] = operation_data.value
            self.log_entry_op_result.append((True, "success"))
        else:
            self.log_entry_op_result.append((False, "no value with such key"))

    def handle_cas(self, operation_data):
        if operation_data.key in self.kv_storage:
            if self.kv_storage[operation_data.key] != operation_data.expectedValue:
                self.log_entry_op_result.append((False, "value in storage doesn't match with expected value"))
            else:
                self.kv_storage[operation_data.key] = operation_data.newValue
                self.log_entry_op_result.append((True, "success"))
        else:
            self.log_entry_op_result.append((False, "no value with such key"))

    def handle_delete(self, operation_data):
        if operation_data.key in self.kv_storage:
            del self.kv_storage[operation_data.key]
            self.log_entry_op_result.append((True, "success"))
        else:
            self.log_entry_op_result.append((False, "no value with such key"))

    def process_new_entries(self, entries):
        for entry in entries:
            print(f"new entry processed:\n{entry}")
            operation_type = entry.WhichOneof("operation")
            operation_data = getattr(entry, operation_type)

            handler = self.handlers[operation_type]
            handler(operation_data)

    def get_operation_result(self, entry_id):
        return self.log_entry_op_result[entry_id - 1]
