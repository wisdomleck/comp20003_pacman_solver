# Simple test script for assignment 2

function runTest
{
  # $1 - Test file
  # $2 - Propagation
  # $3 - Budget
  # $4 - Timeout length (base, seconds)
  TIMEOUT_LENGTH=$(python -c "print(int($4 * $TIMEOUT_MULTIPLIER))")
  rm "$SCRATCH""_regOut.txt" "$SCRATCH""_valgrind.txt"
  echo "[$(date +'%Y/%m/%d %H:%M:%S')]" "timeout """"$TIMEOUT_LENGTH"""" valgrind """"$PACMAN"""" """"$PREFIX""$1"""" """"$AIMODE"""" """"$2"""" """"$3"""""
  timeout "$TIMEOUT_LENGTH" valgrind "$PACMAN" "$PREFIX""$1" "$AIMODE" "$2" "$3" 2> "$SCRATCH""_valgrind.txt" | tee "$SCRATCH""_regOut.txt"
  grep "Good bye!" "$SCRATCH""_regOut.txt" > /dev/null
  if [[ "$?" = "0" ]]
  then
    echo "[$(date +'%Y/%m/%d %H:%M:%S')]" "Passed test!"
  else
    echo "[$(date +'%Y/%m/%d %H:%M:%S')]" "Failed test $1"
  fi
}

PACMAN="./pacman"
PREFIX=$(dirname "$0")"/"
AIMODE="ai"
# Timeout multiplier, can be used to adjust timing wholesale if your program
# takes too long under load.
TIMEOUT_MULTIPLIER="2"
if [[ -z "$SCRATCH" ]]
then
  SCRATCH="/dev/shm/"
fi

echo "Assignment 2 Beta Testing Script"
echo "--------------------------------"
echo "[$(date +'%Y/%m/%d %H:%M:%S')]" "Beginning test script."

make -B

echo "[$(date +'%Y/%m/%d %H:%M:%S')]" "Beginning tests."

runTest abyss100 max 100 60
runTest abyss100 avg 100 60
runTest column100 max 100 60
runTest column100 avg 100 60
runTest columnTrail max 100 60
runTest columnTrail avg 100 60
runTest line avg 5 60
runTest line max 5 60
runTest line avg 1000 60
runTest line max 1000 60
runTest snake100 avg 100 60
runTest snake100 max 100 60
runTest snakeConfine max 5 60
runTest snakeConfine avg 5 60
runTest basicDown max 5 60
runTest basicDown avg 5 60
runTest basicUp max 5 60
runTest basicUp avg 5 60
runTest basicLeft max 5 60
runTest basicLeft avg 5 60
runTest basicRightTwo max 5 60
runTest basicRightTwo avg 5 60
