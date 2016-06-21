#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfstat"
USAGE="Usage: gfstat [OPTION]... URL"
USAGE_ERROR="gfstat: missing operand"

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

@test "uri only" {
        run $CMD "glfs://"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfstat: glfs://: Invalid argument" ]]
}

@test "uri with host" {
        run $CMD "glfs://host"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfstat: glfs://host: Invalid argument" ]]
}

@test "uri with host trailing slash" {
        run $CMD "glfs://host/"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfstat: glfs://host/: Invalid argument" ]]
}

@test "uri with host and empty volume" {
        run $CMD "glfs://host//"

        [ "$status" -eq 1 ]
        [[ "$output" =~ "gfstat: glfs://host//: Invalid argument" ]]
}

@test "uri with host and volume" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME"
        echo $output > /tmp/output

        [ "$status" -eq 0 ]
        [[ "$output" =~ "File: \`/'" ]]
}

@test "uri with host and volume with trailing slash" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME/"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "File: \`/'" ]]
}

@test "stat file" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/$TEST_FILE_SMALL"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "File: \`$ROOT_DIR/$TEST_FILE_SMALL'" ]]
}

@test "stat non-existant path" {
        run $CMD "glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/does_not_exist"

        [ "$status" -eq 1 ]
        [ "$output" == "gfstat: cannot stat \`glfs://$HOST/$GLUSTER_VOLUME$ROOT_DIR/does_not_exist': No such file or directory" ]
}
