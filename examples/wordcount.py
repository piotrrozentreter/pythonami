# Read a file, count vowels and unique letter types (case-insensitive).

def main():
    filename = input("Enter the filename to read: ")

    try:
        with fopen(filename, 'r') as f:
            text = fread(f, 2147483647)
    except IOError:
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

main()
