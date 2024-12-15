import random
import typing as t
from http import HTTPStatus

import requests
from requests import Response

import nodes


class Client:
    def __init__(self) -> None:
        self.current_leader_address = self.get_random_node_address()

    def send_request(
        self,
        method: t.Callable,
        suffix: str,
        params: t.Dict = {},
        json_data: t.Dict = {},
    ) -> Response:
        while True:
            try:
                result: Response = method(
                    self.current_leader_address + suffix,
                    params=params,
                    json=json_data,
                    timeout=0.3,
                )
                json: t.Dict[str, str] = result.json()
                if result.status_code == HTTPStatus.PERMANENT_REDIRECT:
                    leader_id: int = int(json["leader_id"])
                    if leader_id == -1:
                        self.current_leader_address = self.get_random_node_address()
                    else:
                        self.current_leader_address = nodes.GetHTTPAddress(leader_id)
                else:
                    return result

            except requests.exceptions.RequestException:
                self.current_leader_address = self.get_random_node_address()

    def get_random_node_address(self) -> str:
        return nodes.GetHTTPAddress(random.randrange(0, nodes.nodes_cnt))

    def create(self, key: str, value: str) -> bool:
        result: Response = self.send_request(
            method=requests.post,
            suffix="/kv_storage",
            json_data={"key": key, "value": value},
        )
        if result.status_code != HTTPStatus.OK:
            print("[create_error]: " + result.json()["error"])
        return result.status_code == HTTPStatus.OK

    def read(self, key: str) -> str | None:
        while True:
            result: Response = self.send_request(
                method=requests.get,
                suffix="/kv_storage/" + key,
                params={"stage": "initial"},
            )
            json: t.Dict[str, str] = result.json()
            node_id: int = int(json["node_id"])
            operation_id: str = json["operation_id"]
            print(f"redirected to: {node_id}")
            try:
                read_result: Response = requests.get(
                    nodes.GetHTTPAddress(node_id) + "/kv_storage/" + key,
                    params={"stage": "final", "operation_id": operation_id},
                )
                if read_result.status_code == HTTPStatus.OK:
                    return read_result.json()["value"]
                else:
                    print("[read_error]: " + read_result.json()["error"])
                    return None

            except requests.exceptions.RequestException:
                pass

    def update(self, key: str, value: str) -> bool:
        result: Response = self.send_request(
            method=requests.put,
            suffix="/kv_storage",
            json_data={"key": key, "value": value},
        )
        if result.status_code != HTTPStatus.OK:
            print("[update_error]: " + result.json()["error"])
        return result.status_code == HTTPStatus.OK

    def cas(self, key: str, expected_value: str, new_value: str) -> bool:
        result: Response = self.send_request(
            method=requests.patch,
            suffix="/kv_storage",
            json_data={
                "key": key,
                "expected_value": expected_value,
                "new_value": new_value,
            },
        )
        if result.status_code != HTTPStatus.OK:
            print("[cas_error]: " + result.json()["error"])
        return result.status_code == HTTPStatus.OK

    def delete(self, key: str) -> bool:
        result: Response = self.send_request(
            method=requests.delete, suffix="/kv_storage/" + key
        )
        if result.status_code != HTTPStatus.OK:
            print("[delete_error]: " + result.json()["error"])
        return result.status_code == HTTPStatus.OK


if __name__ == "__main__":
    client: Client = Client()
    while True:
        print("Type next operation type:")
        op_type: str = input()
        if op_type == "create":
            print("Type key and value:")
            key, value = input().split()
            print("Success: ", client.create(key=key, value=value))
        elif op_type == "read":
            print("Type key:")
            key = input()
            print("Value: ", client.read(key=key))
        elif op_type == "update":
            print("Type key and value:")
            key, value = input().split()
            print("Success: ", client.update(key=key, value=value))
        elif op_type == "cas":
            print("Type key, expected and new value:")
            key, expected_value, new_value = input().split()
            print(
                "Success: ",
                client.cas(key=key, expected_value=expected_value, new_value=new_value),
            )
        elif op_type == "delete":
            print("Type key:")
            key = input()
            print("Success: ", client.delete(key=key))
