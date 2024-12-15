#! /bin/sh
source /Users/nikitagolu8ev/programming/tftds/venv/bin/activate
export PYTHONPATH=/Users/nikitagolu8ev/programming/tftds/tasks/raft/rpc:$PYTHONPATH
ps aux | grep server.py | awk '{ print $2; }' | xargs kill
python /Users/nikitagolu8ev/programming/tftds/tasks/raft/tests.py
