#!/usr/bin/env bats

@test "accept: single process" {
  tcpexec 127.0.0.1:9797 echo "test" &
  run nc -w1 127.0.0.1 9797
  expect="test"
  cat <<EOF
|$output|
---
|$expect|
EOF

  [ "$status" -eq 0 ]
  [ "$output" = "$expect" ]
}

@test "accept: multiple processes" {
  tcpexec 127.0.0.1:9797 echo "test" &
  tcpexec 127.0.0.1:9797 echo "test" &

  run nc -w1 127.0.0.1 9797
  expect="test"
  cat <<EOF
|$output|
---
|$expect|
EOF

  [ "$status" -eq 0 ]
  [ "$output" = "$expect" ]

  run nc -w1 127.0.0.1 9797
  expect="test"
  cat <<EOF
|$output|
---
|$expect|
EOF

  [ "$status" -eq 0 ]
  [ "$output" = "$expect" ]
}
