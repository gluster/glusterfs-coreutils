#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfcat"
USAGE_ERROR="gfcat: missing operand"
USAGE="Usage: gfcat [OPTION]... URL"

setup() {
        TEST_CAT_DIR=$(mktemp -d --tmpdir="$GLUSTER_MOUNT_DIR$ROOT_DIR")
        TEST_CAT_DIR=$(basename "$TEST_CAT_DIR")
}

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_CAT_DIR"
}

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

@test "invalid port flag" {
        run $CMD "-p" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcat: glfs://: Invalid argument" ]]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcat: glfs://host: Invalid argument" ]]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcat: glfs://host/: Invalid argument" ]]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcat: glfs://host//: Invalid argument" ]]
}

@test "uri with host and volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://$HOST/$GLUSTER_VOLUME: Is a directory" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://$HOST/$GLUSTER_VOLUME/: Is a directory" ]
}

@test "uri with host, volume, and empty path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME//"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://$HOST/$GLUSTER_VOLUME//: Is a directory" ]
}

@test "cat small file" {
        result=$($CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL" | md5sum | awk '{print $1}')

        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "cat medium file" {
        result=$($CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_MEDIUM" | md5sum | awk '{print $1}')

        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "cat large file" {
        result=$($CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_MEDIUM" | md5sum | awk '{print $1}')

        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "cat directory" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_CAT_DIR"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_CAT_DIR: Is a directory" ]
}

@test "cat with host that does not exist" {
        run $CMD "glfs://host/volume/file"

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://host/volume/file: Transport endpoint is not connected" ]
}

@test "cat file that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file";

        [ "$status" -eq 1 ]
        [ "$output" == "gfcat: glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_file: No such file or directory" ]
}
