print("String Reverse and ASCII Test")
print()

text = input("Enter a string: ")

# Reverse string
reversed_text = ""
i = len(text) - 1

while i >= 0:
    reversed_text = reversed_text + text[i]
    i = i - 1

print()
print("Original:", text)
print("Reversed:", reversed_text)

print()
print("Character Analysis")
print("------------------")

ascii_product = 1

i = 0
while i < len(text):
    ch = text[i]
    code = ord(ch)

    print(ch, "->", code)

    ascii_product = ascii_product * code

    i = i + 1

print()
print("ASCII multiplication result:", ascii_product)