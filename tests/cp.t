#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfcp"
USAGE="Usage: gfcp [OPTION]... SOURCE DEST"
USAGE_ERROR="gfcp: missing operand"

setup() {
        TEMP_FILE=$(mktemp)
}

teardown() {
        rm -rf "$TEMP_FILE"
        rm -rf "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test"
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
        [ "$output" == "gfcp: invalid port number: \"test\"" ]
}

@test "source uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "source uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "source uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "source uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "source uri with host and volume" {
        run $CMD "glfs://host/volume"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "source uri with host and volume with trailing slash" {
        run $CMD "glfs://host/volume/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "$USAGE_ERROR" ]]
}

@test "cp the same source and destination" {
        run $CMD "path" "path"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcp: source and destination are the same: Invalid argument" ]]
}

@test "cp local path to local path" {
        run $CMD "$GLUSTER_BRICK_DIR$ROOT_DIR/$TEST_FILE_SMALL" "test"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfcp: local source and destination: Invalid argument" ]]
}

@test "cp small local file to remote destination" {
        run $CMD "$GLUSTER_BRICK_DIR$ROOT_DIR/$TEST_FILE_SMALL" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "cp medium local file to remote destination" {
        run $CMD "$GLUSTER_BRICK_DIR$ROOT_DIR/$TEST_FILE_MEDIUM" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "cp large local file to remote destination" {
        run $CMD "$GLUSTER_BRICK_DIR$ROOT_DIR/$TEST_FILE_LARGE" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_LARGE_HASH" ]
}

@test "cp small remote file to local destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL" "$TEMP_FILE"
        result=$(md5sum "$TEMP_FILE" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "cp medium remote file to local destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_MEDIUM" "$TEMP_FILE"
        result=$(md5sum "$TEMP_FILE" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "cp large remote file to local destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_LARGE" "$TEMP_FILE"
        result=$(md5sum "$TEMP_FILE" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_LARGE_HASH" ]
}

@test "cp small remote file to remote destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "cp medium remote file to remote destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_MEDIUM" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "cp large remote file to remote destination" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_LARGE" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfcp_test"
        result=$(md5sum "$GLUSTER_BRICK_DIR$ROOT_DIR/gfcp_test" | awk '{print $1}')

        [ "$status" -eq 0 ]
        [ "$result" == "$TEST_FILE_LARGE_HASH" ]
}
