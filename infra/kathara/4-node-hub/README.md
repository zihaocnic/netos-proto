# Kathara 4-Node Hub Demo

Topology:

```
node1 -- node2 -- node3
             |
           node4
```

- node1 seeds `alpha`
- node2 requests `alpha`
- node3 + node4 request `alpha` later (hop-through via node2)

Run:

```bash
python -m kathara lstart --noterminals -d infra/kathara/4-node-hub
```

Stop:

```bash
python -m kathara lclean -d infra/kathara/4-node-hub
```

Verify:

```bash
python -m kathara exec node1 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 10'
python -m kathara exec node2 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 40'
python -m kathara exec node3 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
python -m kathara exec node4 -- sh -lc 'grep -E "req_state|data_state" -n /var/log/startup.log | tail -n 20'
```
