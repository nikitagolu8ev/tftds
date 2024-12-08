nodes_cnt = 3
majority_cnt = nodes_cnt // 2 + 1

ports = [50000 + i for i in range(nodes_cnt)]
hosts = ["localhost"] * nodes_cnt

def GetAddress(node_id):
    return hosts[node_id] + ":" + str(ports[node_id])

def GetAddresses():
    return [GetAddress(i) for i in range(nodes_cnt)]
