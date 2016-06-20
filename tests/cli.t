#!/usr/bin/env bats

CMD="$CMD_PREFIX $BUILD_DIR/bin/gfcli"
URL="glfs://$HOST/$GLUSTER_VOLUME"
USAGE="Usage: gfcli [OPTION]... [URL]"

@test "short help flag" {
        run $CMD "-h"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "$USAGE" ]]
}

@test "long help flag" {
        run $CMD "--help"

        [ "$status" -eq 0 ]
        [[ "$output" =~ "$USAGE" ]]
}

@test "quit" {
        run bash -c "echo \"quit\" | $CMD"

        [ "$status" -eq 0 ]
        [ "$output" == "gfcli> quit" ]
}

@test "connect without disconnect" {
        run bash -c "echo -e \"connect $URL\nquit\" | $CMD"

        [ "$status" -eq 0 ]
        [[ "$output" = `echo -e "gfcli> connect $URL\ngfcli \($HOST/$GLUSTER_VOLUME\)> quit"` ]]
}

@test "disconnect without connect" {
        run bash -c "echo -e \"disconnect\nquit\" | $CMD"

        [ "$status" -eq 0 ]
        [[ "$output" = `echo -e "gfcli> disconnect\ngfcli> quit"` ]]
}

@test "connect followed by disconnect" {
        run bash -c "echo -e \"connect glfs://$HOST/$GLUSTER_VOLUME\ndisconnect\nquit\" | $CMD"

        [ "$status" -eq 0 ]
        [[ "$output" = `echo -e "gfcli> connect $URL\ngfcli \($HOST/$GLUSTER_VOLUME\)> disconnect\ngfcli> quit"` ]]
}

@test "connect, disconnect, connect, quit" {
        run bash -c "echo -e \"connect $URL\ndisconnect\nconnect $URL\nquit\" | $CMD"

        echo $output > /tmp/output
        [ "$status" -eq 0 ]
        [[ "$output" = `echo -e "gfcli> connect $URL\ngfcli \($HOST/$GLUSTER_VOLUME\)> disconnect\ngfcli> connect $URL\ngfcli \($HOST/$GLUSTER_VOLUME\)> quit"` ]]
}
