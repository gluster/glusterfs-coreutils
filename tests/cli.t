#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfcli"
USAGE="Usage: gfcli [-p|--port PORT] [-h|--help] [-v|--version] [glfs://<host>/<volume>]"

@test "short help flag" {
        run $CMD "-h"

        [ "$status" -eq 0 ]
        [ "$output" == "$USAGE" ]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 0 ]
        [ "$output" == "$USAGE" ]
}

@test "quit" {
        echo "quit" | $CMD

        [ "$?" -eq 0 ]
}

@test "connect without disconnect" {
        echo -e "connect glfs://$HOST/$GLUSTER_VOLUME\nquit" | $CMD

        [ "$?" -eq 0 ]
}

@test "disconnect without connect" {
        echo -e "disconnect\nquit" | $CMD

        [ "$?" -eq 0 ]
}

@test "connect followed by disconnect" {
        echo -e "connect glfs://$HOST/$GLUSTER_VOLUME\ndisconnect\nquit" | $CMD

        [ $? -eq 0 ]
}

@test "connect, disconnect, connect, quit" {
        URL="glfs://$HOST/$GLUSTER_VOLUME"
        echo -e "connect $URL\ndisconnect\nconnect $URL\nquit" | $CMD

        [ $? -eq 0 ]
}
