#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gftail"
USAGE="Usage: gftail [OPTION]... URL"
USAGE_ERROR="gftail: missing operand"
BASE_URL="glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

@test "no arguments" {
        run $CMD

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "$USAGE" ]]
}

@test "invalid bytes parameter - negative int" {
        run $CMD "-c" "-10"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: invalid number of bytes: \"-10\"" ]
}

@test "invalid bytes parameter - string" {
        run $CMD "-c" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: invalid number of bytes: \"test\"" ]
}

@test "invalid lines parameter" {
        run $CMD "-n" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: invalid number of lines: \"test\"" ]
}

@test "invalid sleep interval parameter" {
        run $CMD "-s" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: invalid sleep interval: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gftail: glfs://: Invalid argument" ]]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gftail: glfs://host: Invalid argument" ]]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gftail: glfs://host/: Invalid argument" ]]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gftail: glfs://host//: Invalid argument" ]]
}

@test "uri with host and volume" {
        run $CMD "glfs://host/volume"
        echo $output > /tmp/output

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: glfs://host/volume: Transport endpoint is not connected" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://host/volume/"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: glfs://host/volume/: Transport endpoint is not connected" ]
}

@test "tail file" {
        result=$($CMD -c 10 "$BASE_URL/$TEST_FILE_MEDIUM" | md5sum | awk '{print $1}')
        expected_result=$(tail -c 10 "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_MEDIUM" | md5sum | awk '{print $1}')

        [ "$result" == "$expected_result" ]
}

@test "tail directory" {
        run $CMD "$BASE_URL/$TEST_DIR"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: error reading \`$BASE_URL/$TEST_DIR': Is a directory" ]
}

@test "tail non-existant path" {
        run $CMD "$BASE_URL/does_not_exist"

        [ "$status" -eq 1 ]
        [ "$output" == "gftail: cannot open \`$BASE_URL/does_not_exist' for reading: No such file or directory" ]
}

@test "tail zero lines" {
        run $CMD "-n0" "$BASE_URL/$TEST_FILE_SMALL"

        [ "$status" -eq 0 ]
}

@test "tail one line" {
        result=$($CMD -n 1 "$BASE_URL/$TEST_FILE_MEDIUM" | wc -l)

        [ $? -eq 0 ]
        [ "$result" == "1" ]
}

@test "tail number of lines in file" {
        lines=$(sed -n '$=' "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_SMALL")
        run $CMD -n "$lines" "$BASE_URL/$TEST_FILE_SMALL"

        [ "$status" -eq 0 ]
}

@test "tail more than number of lines in file" {
        run $CMD -n 1000000 "$BASE_URL/$TEST_FILE_SMALL"

        [ "$status" -eq 0 ]
}
