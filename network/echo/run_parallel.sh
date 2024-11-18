#!/bin/bash

# Check if at least two arguments are provided
if [ "$#" -lt 2 ]; then
  echo "Usage: $0 <num> <command> [args...]"
  exit 1
fi

# Delete the existing log file
rm -f parallel_run.log

NUM=$1
shift
COMMAND=("$@")  # Store command and its arguments as an array

echo "Running command: ${COMMAND[*]}"
echo "Number of parallel instances: $NUM"

for i in $(seq 1 "$NUM"); do
  echo "Starting instance $i..."
  "${COMMAND[@]}" >> "parallel_run.log" &
done

echo "All instances started. Waiting for them to finish..."
# Wait for all background processes to complete
wait
echo "All parallel processes have completed."
