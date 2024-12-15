import typing as t

nodes_cnt: int = 5
majority_cnt: int = nodes_cnt // 2 + 1

grpc_ports: t.List[int] = [50000 + i for i in range(nodes_cnt)]
grpc_hosts: t.List[str] = ["localhost"] * nodes_cnt
http_ports: t.List[int] = [5000 + i for i in range(nodes_cnt)]
http_hosts: t.List[str] = ["http://127.0.0.1"] * nodes_cnt


def GetGRPCAddress(node_id: int) -> str:
    return grpc_hosts[node_id] + ":" + str(grpc_ports[node_id])


def GetGRPCAddresses() -> t.List[str]:
    return [GetGRPCAddress(i) for i in range(nodes_cnt)]


def GetHTTPAddress(node_id: int) -> str:
    return http_hosts[node_id] + ":" + str(http_ports[node_id])


def GetHTTPAddresses() -> t.List[str]:
    return [GetHTTPAddress(i) for i in range(nodes_cnt)]
