#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfput"
USAGE_ERROR=$"gfput: missing operand
Try --help for more information."
USAGE="Usage: gfput [OPTION]... URL
Put data from standard input on a remote Gluster volume.

  -a, --append		append data to the end of the file
  -o, --overwrite	overwrite the existing file
  -r, --parents		no error if existing, make parent directories as needed
  -p, --port=PORT	specify the port on which to connect
      --help     display this help and exit
      --version  output version information and exit

Examples:
  gfput glfs://localhost/groot/file
	Write the contents of standard input to /file on the
	Gluster volume of groot on host localhost.
  gfput -r glfs://localhost/groot/path/to/file
	Write the contents of standard input to /file on the
	Gluster volume of groot on host localhost, creating the
	parent directories as necessary."

teardown() {
        rm -f "$GLUSTER_MOUNT_DIR$ROOT_DIR/gfput_test"
}

@test "no arguments" {
        run $CMD

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE_ERROR" ]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 0 ]
        [ "$output" == "$USAGE" ]
}

@test "invalid port flag" {
        run $CMD "-p" "test"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: invalid port number: \"test\"" ]
}

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://: Invalid argument" ]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://host: Invalid argument" ]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://host/: Invalid argument" ]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://host//: Invalid argument" ]
}

@test "uri with host and volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://$HOST/$GLUSTER_VOLUME: Is a directory" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME/"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://$HOST/$GLUSTER_VOLUME/: Is a directory" ]
}

@test "uri with host, volume, and empty path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME//"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://$HOST/$GLUSTER_VOLUME//: File exists" ]
}

@test "put small file" {
        cat "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_SMALL" | $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfput_test";
        result=$(md5sum $GLUSTER_MOUNT_DIR$ROOT_DIR/gfput_test | awk '{print $1}')

        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "put medium file" {
        cat "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_MEDIUM" | $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfput_test";
        result=$(md5sum $GLUSTER_MOUNT_DIR$ROOT_DIR/gfput_test | awk '{print $1}')

        [ "$result" == "$TEST_FILE_MEDIUM_HASH" ]
}

@test "put large file" {
        cat "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_LARGE" | $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/gfput_test";
        result=$(md5sum $GLUSTER_MOUNT_DIR$ROOT_DIR/gfput_test | awk '{print $1}')

        [ "$result" == "$TEST_FILE_LARGE_HASH" ]
}

@test "put file into subdir that does not exist with parent flag" {
        cat "$GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_FILE_SMALL" | $CMD "-r" "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_DIR/gfput_test";
        result=$(md5sum $GLUSTER_MOUNT_DIR$ROOT_DIR/$TEST_DIR/gfput_test | awk '{print $1}')

        [ "$result" == "$TEST_FILE_SMALL_HASH" ]
}

@test "put file into subdir that does not exist without parent flag" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/subdir/file"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/subdir/file: No such file or directory" ]
}

@test "put file into subdir that does exist" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_DIR"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_DIR: File exists" ]
}

@test "put with host that does not exist" {
        run $CMD "glfs://host/volume/file"

        [ "$status" -eq 1 ]
        [ "$output" == "gfput: glfs://host/volume/file: Transport endpoint is not connected" ]
}
