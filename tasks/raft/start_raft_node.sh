#! /bin/sh
source /Users/nikitagolu8ev/programming/tftds/venv/bin/activate
export PYTHONPATH=/Users/nikitagolu8ev/programming/tftds/tasks/raft/rpc:$PYTHONPATH
python /Users/nikitagolu8ev/programming/tftds/tasks/raft/server.py $1
