#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfmkdir"
USAGE="Usage: gfmkdir [OPTION]... URL"
USAGE_ERROR="gfmkdir: missing operand"

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR$ROOT_DIR/test_dir"
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
        [ "$output" == "gfmkdir: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfmkdir: glfs://: Invalid argument" ]]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfmkdir: glfs://host: Invalid argument" ]]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfmkdir: glfs://host/: Invalid argument" ]]
}


@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfmkdir: glfs://host//: Invalid argument" ]]
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
        [ -d "$GLUSTER_MOUNT_DIR$ROOT_DIR/test_dir" ] && status=0

        [ "$status" -eq 0 ]
}

@test "mkdir directory that does not exist with parents flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/test_dir"
        [ -d "$GLUSTER_MOUNT_DIR$ROOT_DIR/test_dir" ] && status=0

        [ "$status" -eq 0 ]
}

@test "mkdir nested directory that does not exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level"

        [ "$status" -eq 1 ]
        [ "$output" == "gfmkdir: cannot create directory \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level': No such file or directory" ]
}

@test "mkdir nested directory that does not exist with parents flag" {
        run $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/first_level/second_level"
        [ -d "$GLUSTER_MOUNT_DIR$ROOT_DIR/first_level/second_level" ] && status=0

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
