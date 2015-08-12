#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfmkdir"
USAGE="Usage: gfmkdir [-r|--parents] [-p|--port PORT] [-h|--help] [-v|--version] glfs://<host>/<volume>/<path>"

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/test_dir"
}

@test "no arguments" {
        run $CMD

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "short help flag" {
        run $CMD "-h"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "invalid port flag" {
        run $CMD "-p" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: invalid port number: \"test\"" ]
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

@test "uri with host and volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME': File exists" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME/': File exists" ]
}

@test "uri with host, volume, and empty path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME//"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME//': File exists" ]
}

@test "mkdir directory that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/test_dir"

        [ "$status" -eq 0 ]
}

@test "mkdir directory that does not exist with parents flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/test_dir"

        [ "$status" -eq 0 ]
}

@test "mkdir nested directory that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level': No such file or directory" ]
}

@test "mkdir nested directory that does not exist with parents flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level"

        [ "$status" -eq 0 ]
}

@test "mkdir file that already exists" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL': File exists" ]
}

@test "mkdir directory that already exists" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_DIR"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_DIR': File exists" ]
}
