#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfstat"
USAGE="Usage: gfstat [-L|--deference] [-p|--port PORT] [-h|--help] [-v|--version] glfs://<host>/<volume>/<path>"

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
        run $CMD "glfs://host/volume"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://host/volume/"

        [ "$status" -eq 1 ]
        [ "$output" == "$USAGE" ]
}

@test "stat file" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL"

        [ "$status" -eq 0 ]
}

@test "stat non-existant path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/does_not_exist"

        [ "$status" -eq 1 ]
        [ "$output" == "gfstat: cannot stat \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/does_not_exist': No such file or directory" ]
}
