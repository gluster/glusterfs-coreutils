#!/bin/bash

# Test suite execution script for the gluster-coreutils project. The execution
# script is responsible for verifying the system environment in which the tests
# will be executed and setting up and validating (as appropriate) the test
# environment against which the tests will be executed.
#
# The tests are implemented using the Test Anything Protocol (testanything.org)
# In this case, the test consumer is the Perl `prove` test harness and the test
# producer is the `bats` utility (github.com/sstephenson/bats).
#
# There are three environment variables that the script tests for which will
# influence the behavior of its execution: GLUSTER_VOLUME, GLUSTER_BRICK_DIR,
# and GLUSTER_MOUNT_DIR. If all three variables are set, the script will skip
# setting up a temporary Gluster volume of its own and will instead use the
# provided values. GLUSTER_VOLUME must be a valid Gluster volume that is
# started, GLUSTER_BRICK_DIR should point to the directory that backs the
# volume, and GLUSTER_MOUNT_DIR should point to the mount point on which the
# volume is mounted (via FUSE). Note that as of this writing, the script does
# not do extensive validation of these provided values, so it is up to the
# user to ensure that they are sane values. Otherwise, the behavior of the
# script (and tests) is likely to be erratic and non-conclusive.

# prevent unnecessary headaches
set -u

DIR=$(cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd)
GLUSTER_DAEMON_NOT_RUNNING=true
GLUSTER_DAEMON_PID=-1
with_valgrind="no"

export BUILD_DIR="$DIR/build"
export CLI="gluster --mode=script --wignore"
export CMD_PREFIX=""
export HOST="::1"
export VALGRIND="valgrind --input-fd=3 --quiet --error-exitcode=2 \
        --leak-check=full --suppressions=$DIR/tests/util/valgrind.suppressions"

source tests/util/log.rc

function check_root() {
        if [[ $(id -u) -ne 0 ]]; then
                error "Test needs to be executed with superuser privileges.\n"
                exit 1
        fi
}

function check_environment() {
        highlight "==> Executing preflight\n"

        MISSING=()
        for cmd in bats gluster glusterd glusterfsd prove; do
                command -v $cmd > /dev/null || MISSING+=($cmd)
        done

        if [[ ${#MISSING[@]} -ne 0 ]]; then
                error "This system is missing the required tools:\n"

                for pkg in $MISSING; do
                        echo "* $pkg"
                done

                exit 1
        fi

        if [[ ! -f $BUILD_DIR/bin/gfcli ]]; then
                error "Utilities must be built before executing tests.\n"
                exit 1
        fi

        if [[ ! -d $DIR/tests ]]; then
                error "No tests directory present.\n"
                exit 1
        fi

        info "Checking for running daemon: "
        GLUSTER_DAEMON_PID=$(pgrep glusterd)
        if [[ $? -eq 0 ]]; then
                printf "yes\n"
                GLUSTER_DAEMON_NOT_RUNNING=false
        else
                printf "no\n"
                info "Starting gluster daemon: "
                glusterd
                if [[ $? -eq 0 ]]; then
                        printf "started\n"
                else
                        printf "failed!\n"
                        error "failed to start gluster daemon\n"
                        exit 1
                fi
        fi
}

function setup_temporary_test_environment() {
        highlight "==> Executing setup\n"

        export ROOT_DIR=""

        info "Creating temporary brick directory: "
        export GLUSTER_BRICK_DIR=$(mktemp -d --tmpdir=/tmp BRICKXXXXXX)
        printf "$GLUSTER_BRICK_DIR\n"


        info "Creating temporary gluster volume: "
        export GLUSTER_VOLUME=$(basename $GLUSTER_BRICK_DIR)
        RESULT=$($CLI volume create $GLUSTER_VOLUME $HOST:$GLUSTER_BRICK_DIR force)
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to create temporary volume $GLUSTER_VOLUME\n"
                exit 1
        else
                printf "$GLUSTER_VOLUME\n"
        fi

        generate_data

        info "Starting temporary gluster volume: "
        RESULT=$($CLI volume start $GLUSTER_VOLUME force)
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to start temporary volume $GLUSTER_VOLUME\n"
                exit 1
        else
                printf "$GLUSTER_VOLUME\n"
        fi

        info "Creating temporary mount directory: "
        export GLUSTER_MOUNT_DIR=$(mktemp -d --tmpdir=/tmp MNTXXXXXXX)
        printf "$GLUSTER_MOUNT_DIR\n"

        info "Mounting temporary volume: "
        mount -t glusterfs "$HOST:/$GLUSTER_VOLUME" $GLUSTER_MOUNT_DIR
        if [[ $? -eq 0 ]]; then
                printf "$GLUSTER_MOUNT_DIR\n"
        else
                printf "failed!\n"
                error "failed to mount temporary brick $GLUSTER_VOLUME\n"
                exit 1
        fi
}

function setup_test_environment() {
        ROOT_DIR=$(mktemp -d --tmpdir=$GLUSTER_BRICK_DIR)
        export ROOT_DIR="/$(basename $ROOT_DIR)"

        generate_data
}

function generate_data() {
        info "Generating test files and directories:\n"
        TEST_DIR=$(mktemp -d --tmpdir="$GLUSTER_BRICK_DIR$ROOT_DIR")
        export TEST_DIR=$(basename $TEST_DIR)
        printf "* $TEST_DIR [directory]\n"

        TEST_FILE_SMALL=$(mktemp --tmpdir="$GLUSTER_BRICK_DIR$ROOT_DIR")
        RESULT=$(dd if=/dev/urandom of=$TEST_FILE_SMALL bs=1024 count=1 2>&1)
        export TEST_FILE_SMALL_HASH=$(md5sum $TEST_FILE_SMALL | awk '{print $1}')
        export TEST_FILE_SMALL=$(basename $TEST_FILE_SMALL)
        printf "* $TEST_FILE_SMALL: $TEST_FILE_SMALL_HASH\n"

        TEST_FILE_MEDIUM=$(mktemp --tmpdir="$GLUSTER_BRICK_DIR$ROOT_DIR")
        RESULT=$(dd if=/dev/urandom of=$TEST_FILE_MEDIUM bs=1M count=10 2>&1)
        export TEST_FILE_MEDIUM_HASH=$(md5sum $TEST_FILE_MEDIUM | awk '{print $1}')
        export TEST_FILE_MEDIUM=$(basename $TEST_FILE_MEDIUM)
        printf "* $TEST_FILE_MEDIUM: $TEST_FILE_MEDIUM_HASH\n"

        TEST_FILE_LARGE=$(mktemp --tmpdir="$GLUSTER_BRICK_DIR$ROOT_DIR")
        RESULT=$(dd if=/dev/urandom of=$TEST_FILE_LARGE bs=1M count=100 2>&1)
        export TEST_FILE_LARGE_HASH=$(md5sum $TEST_FILE_LARGE | awk '{print $1}')
        export TEST_FILE_LARGE=$(basename $TEST_FILE_LARGE)
        printf "* $TEST_FILE_LARGE: $TEST_FILE_LARGE_HASH\n"
}

function run_tests() {
        highlight "==> Executing tests\n"

        if [ $with_valgrind == "yes" ]; then
                CMD_PREFIX="$VALGRIND"
        fi

        prove -rf --timer tests
}

function cleanup_temporary_test_environment() {
        highlight "==> Executing test environment cleanup\n"

        info "Unmounting temporary mount directory: "
        umount -fl $GLUSTER_MOUNT_DIR
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to unmount temporary mount directory $GLUSTER_MOUNT_DIR\n"
        else
                printf "$GLUSTER_MOUNT_DIR\n"
        fi

        info "Removing temporary mount directory: "
        rm -rf $GLUSTER_MOUNT_DIR
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to remove temporary mount directory $GLUSTER_MOUNT_DIR\n"
        else
                printf "$GLUSTER_MOUNT_DIR\n"
        fi

        info "Stopping temporary gluster volume: "
        RESULT=$($CLI volume stop $GLUSTER_VOLUME force)
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to stop temporary volume $GLUSTER_VOLUME\n"
        else
                printf "$GLUSTER_VOLUME\n"
        fi

        info "Removing temporary gluster volume: "
        RESULT=$($CLI volume delete $GLUSTER_VOLUME)
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to remove temporary volume $GLUSTER_VOLUME\n"
        else
                printf "$GLUSTER_VOLUME\n"
        fi

        info "Removing temporary brick directory: "
        rm -rf $GLUSTER_BRICK_DIR
        if [[ $? -ne 0 ]]; then
                printf "failed!\n"
                error "failed to remove temporary brick directory $GLUSTER_BRICK_DIR\n"
        else
                printf "$GLUSTER_BRICK_DIR\n"
        fi

        if $GLUSTER_DAEMON_NOT_RUNNING; then
                info "Stopping gluster daemon: "
                RESULT=$(pkill glusterd)
                if [[ $? -ne 0 ]]; then
                        printf "failed!\n"
                        error "failed to stop gluster daemon"
                        exit 1
                else
                        printf "done\n"
                fi
        fi
}

function cleanup_test_environment() {
        highlight "==> Executing test environment cleanup\n"

        rm -rf "$GLUSTER_BRICK_DIR$ROOT_DIR"
        if [[ $? -ne 0 ]]; then
                error "failed to remove test data directory\n"
                exit 1
        fi
}

function main() {
        args=`getopt g "$@"`
        set -- $args
        while [ $# -gt 0 ]; do
                case "$1" in
                        -g)     with_valgrind="yes" ;;
                        --)     shift; break ;;
                esac
                shift
        done

        check_root
        check_environment

        if [[ -z ${GLUSTER_BRICK_DIR+x} || -z ${GLUSTER_MOUNT_DIR+x} || -z ${GLUSTER_VOLUME+x} ]]; then
                setup_temporary_test_environment
                run_tests
                cleanup_temporary_test_environment
        else
                setup_test_environment
                run_tests
                cleanup_test_environment
        fi
}

main "$@"
