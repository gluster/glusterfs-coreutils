#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfls"
USAGE="Usage: gfls [OPTION]... URL"
USAGE_ERROR="gfls: missing operand"

setup() {
        mkdir -p "$GLUSTER_MOUNT_DIR$ROOT_DIR/first/second/third"
}

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/first"
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
        [ "$output" == "gfls: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfls: glfs://: Invalid argument" ]]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfls: glfs://host: Invalid argument" ]]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfls: glfs://host/: Invalid argument" ]]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfls: glfs://host//: Invalid argument" ]]
}

@test "uri with invalid host and volume" {
        run $CMD "glfs://host/volume/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfls: failed to access glfs://host/volume/: Transport endpoint is not connected" ]
}

@test "ls root of volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "first" ]]
}

@test "ls root of volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "first" ]]
}

@test "ls root of volume with all flag" {
        run $CMD "-a" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "first" ]]
}

@test "ls root of volume with recursive flag" {
        run $CMD "-R" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "$ROOT_DIR/first/second/third:" ]]
}

@test "ls root of volume with long flag" {
        run $CMD "-l" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "drwx" ]]
}

@test "ls sub-directory with wildcard matching" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first/*"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "second" ]]
}

@test "ls path that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_directory"

        [ "$status" -eq 1 ]
        [ "$output" == "gfls: failed to access glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_directory: No such file or directory" ]
}
