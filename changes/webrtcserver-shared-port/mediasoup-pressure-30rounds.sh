#!/usr/bin/env bash
set -euo pipefail

cd /root/workspace/mediasoup-cpp

round=0
max_rounds=30
summary=/tmp/pressure-30rounds-summary.log
: > "$summary"
echo "start $(date -Is) max_rounds=$max_rounds" | tee -a "$summary"

while [ "$round" -lt "$max_rounds" ]; do
  round=$((round + 1))
  stamp=$(date +%Y%m%d-%H%M%S)
  run_tag="loop30_${stamp}_r${round}"
  perf_dir="/tmp/perf-${run_tag}"
  log_file="/tmp/pressure-${run_tag}.log"

  echo "round=$round tag=$run_tag start=$(date -Is) perf_dir=$perf_dir log=$log_file" | tee -a "$summary"

  stdbuf -oL -eL node tests/qos_harness/multi_process_pressure.mjs \
    --ws-url=wss://127.0.0.1:9000/ws \
    --http-url=https://127.0.0.1:9000 \
    --container=mediasoup-9000 \
    --rooms-per-process=90 \
    --step=10 \
    --round-ms=10000 \
    --steady-round-ms=20000 \
    --spawn-interval-ms=10000 \
    --steady-rounds=0 \
    --recv-ratio=0.85 \
    --max-processes=5 \
    --prefix="$run_tag" \
    --perf \
    --perf-interval-ms=20000 \
    --perf-output-dir="$perf_dir" \
    2>&1 | tee "$log_file" &
  parent_pid=$!

  reached_450=0
  reached_at=0

  while kill -0 "$parent_pid" 2>/dev/null; do
    payload=$(curl -sk https://127.0.0.1:9000/api/node-load 2>/dev/null || true)
    rooms=$(printf "%s" "$payload" | python3 -c "import sys,json; s=sys.stdin.read().strip(); print(int(json.loads(s).get('rooms',0)) if s else 0)" 2>/dev/null || echo 0)
    ready=$(printf "%s" "$payload" | python3 -c "import sys,json; s=sys.stdin.read().strip(); print(str(bool(json.loads(s).get('ready',False))).lower() if s else 'false')" 2>/dev/null || echo false)
    healthy=$(printf "%s" "$payload" | python3 -c "import sys,json; s=sys.stdin.read().strip(); print(str(bool(json.loads(s).get('healthy',False))).lower() if s else 'false')" 2>/dev/null || echo false)

    echo "round=$round poll=$(date -Is) rooms=$rooms ready=$ready healthy=$healthy" | tee -a "$summary"

    if [ "$rooms" -ge 450 ]; then
      if [ "$reached_450" -eq 0 ]; then
        reached_450=1
        reached_at=$(date +%s)
        echo "round=$round reached450=$(date -Is)" | tee -a "$summary"
      fi

      now=$(date +%s)
      if [ $((now - reached_at)) -ge 30 ]; then
        echo "round=$round stable30s_at_450=1 stopping=$(date -Is)" | tee -a "$summary"
        break
      fi
    fi

    sleep 5
  done

  mapfile -t pids < <(ps -eo pid,args | awk -v tag="$run_tag" '$0 ~ tag && /multi_process_pressure\.mjs|single_worker_pressure\.mjs|tee/ {print $1}')
  mapfile -t perfpids < <(ps -eo pid,args | awk -v pdir="$perf_dir" '$0 ~ pdir && /perf record/ {print $1}')

  if [ "${#pids[@]}" -gt 0 ]; then
    kill -TERM "${pids[@]}" 2>/dev/null || true
  fi
  if [ "${#perfpids[@]}" -gt 0 ]; then
    kill -TERM "${perfpids[@]}" 2>/dev/null || true
  fi

  sleep 3

  if [ "${#pids[@]}" -gt 0 ]; then
    kill -KILL "${pids[@]}" 2>/dev/null || true
  fi
  if [ "${#perfpids[@]}" -gt 0 ]; then
    kill -KILL "${perfpids[@]}" 2>/dev/null || true
  fi

  for _ in $(seq 1 24); do
    payload=$(curl -sk https://127.0.0.1:9000/api/node-load 2>/dev/null || true)
    rooms_after=$(printf "%s" "$payload" | python3 -c "import sys,json; s=sys.stdin.read().strip(); print(int(json.loads(s).get('rooms',-1)) if s else -1)" 2>/dev/null || echo -1)
    if [ "$rooms_after" -eq 0 ]; then
      break
    fi
    sleep 5
  done

  payload=$(curl -sk https://127.0.0.1:9000/api/node-load 2>/dev/null || true)
  rooms_after=$(printf "%s" "$payload" | python3 -c "import sys,json; s=sys.stdin.read().strip(); print(int(json.loads(s).get('rooms',-1)) if s else -1)" 2>/dev/null || echo -1)
  reports=$(find "$perf_dir" -maxdepth 1 -name '*.report' 2>/dev/null | wc -l | tr -d ' ')
  echo "round=$round done=$(date -Is) rooms_after=$rooms_after perf_reports=$reports" | tee -a "$summary"

  sleep 10
done

echo "finished $(date -Is) rounds=$round" | tee -a "$summary"
