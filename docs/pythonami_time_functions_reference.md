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
