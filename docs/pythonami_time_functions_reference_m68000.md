# PythonAmi Time Functions Reference

**Purpose:** Recommended time-related API for implementation in PythonAmi.

Based on the Python `time` module and commonly used timing functionality. citeturn6search33turn6search36

---

# Phase 1 - Essential Functions

## time()

Return current timestamp as seconds since Unix epoch.

```python
now = time()
print(now)
```

Return type:

```python
float
```

---

## time_ns()

Return current timestamp in nanoseconds.

```python
ns = time_ns()
```

Return type:

```python
int
```

---

## sleep(seconds)

Pause program execution.

```python
print('start')
sleep(1.5)
print('end')
```

---

## ctime(seconds=None)

Convert timestamp to a readable string.

```python
print(ctime())
print(ctime(0))
```

Example:

```text
Thu Sep 10 15:30:01 2026
```

---

# Phase 2 - Time Structures

## localtime(seconds=None)

Convert epoch seconds into local time structure. citeturn6search33turn6search36

Example fields:

```python
result.tm_year
result.tm_mon
result.tm_mday
result.tm_hour
result.tm_min
result.tm_sec
result.tm_wday
result.tm_yday
result.tm_isdst
```

Suggested PythonAmi structure:

```python
struct_time
```

---

## gmtime(seconds=None)

Convert epoch seconds into UTC structure.

```python
utc = gmtime()
```

---

## mktime(struct_time)

Convert local time structure back to epoch seconds.

```python
seconds = mktime(localtime())
```

---

# Phase 3 - Formatting and Parsing

## strftime(format, struct_time=None)

Format a date/time structure into text. citeturn6search36turn6search33

```python
strftime('%Y-%m-%d')
strftime('%H:%M:%S')
```

Common format codes:

```text
%Y  four digit year
%y  two digit year
%m  month (01-12)
%d  day (01-31)
%H  hour (00-23)
%M  minute (00-59)
%S  second (00-59)
%j  day of year
%w  weekday
%a  short weekday name
%A  full weekday name
%b  short month name
%B  full month name
%%  percent sign
```

---

## strptime(text, format)

Parse string into time structure.

```python
strptime('2026-09-15', '%Y-%m-%d')
```

---

## asctime(struct_time=None)

Convert a time structure to readable text.

```python
asctime(localtime())
```

Example:

```text
Tue Sep 15 12:30:55 2026
```

---

# Phase 4 - Performance Timing

## monotonic()

Monotonic clock.

Never goes backwards.

Useful for elapsed time measurement. citeturn6search36turn6search37

```python
start = monotonic()
...
end = monotonic()
print(end - start)
```

---

## monotonic_ns()

Nanosecond version.

```python
start = monotonic_ns()
```

---

## perf_counter()

High precision stopwatch timer.

Recommended for benchmarks. citeturn6search36turn6search37

```python
start = perf_counter()
...
elapsed = perf_counter() - start
```

---

## perf_counter_ns()

Nanosecond precision benchmark timer.

```python
perf_counter_ns()
```

---

## process_time()

CPU time used by current process.

Sleep time is excluded.

```python
start = process_time()
```

---

## process_time_ns()

Nanosecond version.

```python
process_time_ns()
```

---

# struct_time Design

Suggested PythonAmi implementation:

```python
struct_time(
    tm_year,
    tm_mon,
    tm_mday,
    tm_hour,
    tm_min,
    tm_sec,
    tm_wday,
    tm_yday,
    tm_isdst
)
```

Example:

```python
s = localtime()
print(s.tm_year)
```

---

# Recommended Implementation Order

## Stage 1 (Minimum Viable)

```text
time
sleep
ctime
```

## Stage 2

```text
localtime
gmtime
mktime
struct_time
```

## Stage 3

```text
strftime
strptime
asctime
```

## Stage 4

```text
monotonic
monotonic_ns
perf_counter
perf_counter_ns
process_time
process_time_ns
```

---

# Test Script

```python
print(time())
print(time_ns())

sleep(1)

print(ctime())

lt = localtime()

print(lt.tm_year)
print(lt.tm_mon)
print(lt.tm_mday)

print(strftime('%Y-%m-%d %H:%M:%S', lt))

start = perf_counter()

sleep(2)

end = perf_counter()

print('Elapsed:', end - start)
```

---

# Implementation Checklist

```text
[ ] time
[ ] time_ns
[ ] sleep
[ ] ctime
[ ] asctime
[ ] localtime
[ ] gmtime
[ ] mktime
[ ] strftime
[ ] strptime
[ ] monotonic
[ ] monotonic_ns
[ ] perf_counter
[ ] perf_counter_ns
[ ] process_time
[ ] process_time_ns
[ ] struct_time
```

For a PythonAmi MVP running on Amiga/M68000, the most important subset is:

```text
time
sleep
ctime
localtime
strftime
perf_counter
```

---

# M68000-Specific Implementation Notes

These notes are **design suggestions** for PythonAmi on Amiga/M68000 systems. They are not part of the Python specification.

## Timer Resolution Limitations

Classic Amiga systems typically do not provide nanosecond precision hardware timers.

Suggested behavior:

```text
perf_counter_ns()
monotonic_ns()
time_ns()
```

may internally use microsecond or tick-based timers and scale the result to nanoseconds.

Example:

```python
actual_resolution = 20000 ns
reported_value   = 123456780000
```

The value is nanosecond-formatted but not necessarily nanosecond-accurate.

---

## 32-bit Overflow Considerations

M68000 systems are fundamentally 32-bit.

Potential issue:

```python
seconds_since_1970
```

may exceed signed 32-bit limits.

Suggested internal representation:

```text
64-bit signed integer
```

for:

```text
time()
time_ns()
monotonic()
perf_counter()
```

Avoid storing timestamps in 32-bit integers.

---

## Y2038 Problem

If timestamps are stored as signed 32-bit seconds:

```text
2038-01-19 03:14:07 UTC
```

becomes the maximum representable value.

Recommendation:

```text
Always use 64-bit timestamps internally.
```

---

## No Floating Point Unit

Many M68000 machines do not contain a 68881/68882 FPU.

Functions returning floating-point values:

```python
time()
perf_counter()
monotonic()
process_time()
```

may become expensive.

Optimization:

```text
time_ns()
perf_counter_ns()
monotonic_ns()
```

can internally use integer arithmetic.

---

## Tick-Based Timing

Many retro systems expose timing as ticks.

Example:

```text
50 Hz PAL
60 Hz NTSC
```

Suggested implementation:

```python
sleep(0.001)
```

rounds upward to the nearest scheduler tick.

Possible actual delays:

```text
20 ms (50 Hz)
16.67 ms (60 Hz)
```

---

## Monotonic Clock Design

Never base:

```python
monotonic()
perf_counter()
```

on wall clock time.

Incorrect:

```text
system date changed
clock adjusted
DST changes
```

These can make elapsed time negative.

Recommended source:

```text
hardware timer
Exec timer device
high resolution tick counter
```

---

## Sleep Edge Cases

Test:

```python
sleep(0)
sleep(0.001)
sleep(-1)
```

Suggested behavior:

```text
sleep(0)      yield scheduler
sleep(<0)     ValueError
```

---

## Local Time and Time Zones

Some Amiga environments may not expose full timezone information.

Possible simplification:

```text
gmtime()
localtime()
```

return identical values when timezone support is unavailable.

Document this for compatibility.

---

## strftime Minimal Subset

Implement first:

```text
%Y
%m
%d
%H
%M
%S
%%
```

These cover most scripts.

Add the remaining format specifiers later.

---

# M68000 Edge-Case Test Suite

```python
print('--- Time Tests ---')

print(time())
print(time_ns())

sleep(0)

try:
    sleep(-1)
except:
    print('PASS negative sleep')

print(ctime(0))

lt = localtime(0)

print(lt.tm_year)
print(lt.tm_mon)
print(lt.tm_mday)

print(strftime('%Y-%m-%d', lt))
```

---

# Long-Running Counter Test

Detect overflow problems.

```python
start = monotonic_ns()

counter = 0

while counter < 1000000:
    counter = counter + 1

end = monotonic_ns()

print(end - start)
```

Expected:

```text
positive integer
```

Never:

```text
negative
wrapped value
```

---

# PythonAmi Recommended MVP

For an initial M68000 interpreter:

```text
struct_time
ctime
localtime
strftime
sleep
time
time_ns
perf_counter
```

Delay implementation of:

```text
process_time
process_time_ns
strptime
```

until the runtime and hardware abstraction layer are stable.
