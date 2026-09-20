# Read a file, count vowels and unique letter types (case-insensitive).
# Relative paths use the process current directory (where pythonami was started).

def main():
    filename = input("Enter the filename to read: ")

    try:
        with fopen(filename, 'r') as f:
            # Large count means "up to EOF"; runtime sizes the buffer from the
            # remaining file length (not the count), so this is Amiga-safe.
            text = fread(f, 2147483647)
    except OSError:
        print("Error reading the file.")
        return
    except ValueError:
        print("Error reading the file.")
        return

    vowels = "aeiouAEIOU"
    vowel_count = 0
    letters_found = set()

    for char in text:
        if char.isalpha():
            letters_found.add(char.lower())
            if char in vowels:
                vowel_count += 1

    print("Number of vowels found:", vowel_count)
    print("Number of different letter types found:", len(letters_found))

    # Dictionary to count occurrences of each letter (case insensitive)
    letter_counts = {}

    for char in text:
        if char.isalpha():
            char_lower = char.lower()
            if char_lower in letter_counts:
                letter_counts[char_lower] += 1
            else:
                letter_counts[char_lower] = 1

    # Output results (f-strings are not in Language Level 0.1)
    if letter_counts:
        print("Letters found in the file and their counts:")
        for letter, count in sorted(letter_counts.items()):
            print("'" + letter + "':", count)
    else:
        print("No letters found in the file.")

main()
