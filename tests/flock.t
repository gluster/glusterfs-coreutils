#!/usr/bin/env bats

GLUSTER_SHELL_CMD="gfcli glfs://$HOST/$GLUSTER_VOLUME"
USAGE="Usage: flock [OPTION]... URL"
GLUSTER_PROMPT="gfcli ($HOST/$GLUSTER_VOLUME)>"

setup() {
        TEST_LOCK_FILE=$(mktemp --tmpdir="$GLUSTER_MOUNT_DIR/$ROOT_DIR")
        TEST_LOCK_FILE=$(basename "$TEST_LOCK_FILE")
}

teardown() {
        rm -rf "$GLUSTER_MOUNT_DIR/$ROOT_DIR/$TEST_LOCK_FILE"
}

@test "Check without options" {
        run bash -c "echo 'flock' | $GLUSTER_SHELL_CMD"

        [[ "$output" =~ "$USAGE" ]]
}

@test "Check --help option" {
        run bash -c "echo 'flock --help' | $GLUSTER_SHELL_CMD"

        [[ "$output" =~ "$USAGE" ]]
}

@test "Try flock on a file without any options" {
        run bash -c "echo \"flock $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD"

        [[ "$output" = `echo -e "$GLUSTER_PROMPT flock $ROOT_DIR/$TEST_LOCK_FILE\n$GLUSTER_PROMPT "` ]]
}

@test "Try normal flock on a file with -s option" {
        run bash -c "echo \"flock -s $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD"

        [[ "$output" = `echo -e "$GLUSTER_PROMPT flock -s $ROOT_DIR/$TEST_LOCK_FILE\n$GLUSTER_PROMPT "` ]]
}

@test "Try flock on a file with --shared long option" {
        run bash -c "echo \"flock --shared $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD"

        [[ "$output" = `echo -e "$GLUSTER_PROMPT flock --shared $ROOT_DIR/$TEST_LOCK_FILE\n$GLUSTER_PROMPT "` ]]
}

@test "Try conflicting write lock on a file with -e and -n options" {

        run bash -c "exec 3>$GLUSTER_MOUNT_DIR/$ROOT_DIR/$TEST_LOCK_FILE; flock -s 3; echo \"flock -n -e $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD; exec 3>&-"

        [[ "$output" =~ "Resource temporarily unavailable" ]]
}

@test "Try acquiring shared lock on a file with read lock already being held" {
        run bash -c "exec 3>$GLUSTER_MOUNT_DIR/$ROOT_DIR/$TEST_LOCK_FILE; flock -s 3; echo \"flock -n -s $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD; exec 3>&-"

        [[ "$output" = `echo -e "$GLUSTER_PROMPT flock -n -s $ROOT_DIR/$TEST_LOCK_FILE\n$GLUSTER_PROMPT "` ]]
}

@test "Check --unlock option" {
        run bash -c "echo \"flock --unlock $ROOT_DIR/$TEST_LOCK_FILE\" | $GLUSTER_SHELL_CMD;"

        [[ "$output" = `echo -e "$GLUSTER_PROMPT flock --unlock $ROOT_DIR/$TEST_LOCK_FILE\n$GLUSTER_PROMPT "` ]]
}
