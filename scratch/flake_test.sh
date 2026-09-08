for i in {1..1000}; do
  build/event_bus_test > /dev/null || { echo "Failed on run $i"; exit 1; }
done
echo "Success!"
