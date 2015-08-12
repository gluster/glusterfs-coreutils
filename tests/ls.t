#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfls"
USAGE="Usage: gfls [-a|--all] [-h|--human-readable] [-l] [-R|--recursive] [-p|--port PORT] [--help] [-v|--version] glfs://<host>/<volume>/<path>"

setup() {
        mkdir -p "$GLUSTER_MOUNT_DIR$ROOT_DIR/first/second/third"
}

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/first"
}

@test "no arguments" {
        run $CMD

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "long help flag" {
        run $CMD

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "invalid port flag" {
        run $CMD "-p" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gfls: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with invalid host and volume" {
        run $CMD "glfs://host/volume/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfls: cannot access glfs://host/volume/: Transport endpoint is not connected" ]
}

@test "ls root of volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
}

@test "ls root of volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/"

        [ "$status" -eq 0 ]
}

@test "ls root of volume with all flag" {
        run $CMD "-a" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
}

@test "ls root of volume with recursive flag" {
        run $CMD "-R" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
}

@test "ls root of volume with long flag" {
        run $CMD "-R" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR"

        [ "$status" -eq 0 ]
}

@test "ls sub-directory with wildcard matching" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first/*"

        [ "$status" -eq 0 ]
}

@test "ls path that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_directory"

        echo "$output" > /tmp/bats.log
        [ "$status" -eq 1 ]
        [ "$output" == "gfls: cannot access glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/no_such_directory: No such file or directory" ]
}
