# Python68K performance benchmark

iterations = 20000
rounds = 200
checksum = 0

print("Python68K performance benchmark")
print("Operations:", iterations * rounds)

start = time_tick()

for round_number in range(rounds):
    for value in range(iterations):
        checksum = (checksum + value * value) % 1000003

elapsed = time_tick() - start

print("Checksum:", checksum)
print("Elapsed milliseconds:", elapsed)