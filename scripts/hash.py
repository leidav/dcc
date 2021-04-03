keywords = [
    'auto', 'break', 'case', 'char', 'const', 'continue', 'default', 'do',
    'double', 'else', 'enum', 'extern', 'float', 'for', 'goto', 'if', 'inline',
    'int', 'long', 'register', 'restrict', 'return', 'short', 'signed',
    'sizeof', 'static', 'struct', 'switch', 'typedef', 'union', 'unsigned',
    'void', 'volatile', 'while', '_Alignas', '_Alignof', '_Bool',
    '_Complex', '_Generic', '_Imaginary', '_Noreturn', '_Static_assert','__constexpr'
]


def djb2(string: str) -> int:
    h = 5381
    for c in string:
        h = ((h * 33) ^ ord(c)) % (2**32)
    return h

def main():
    for keyword in keywords:
        print(f'#define KEYWORD_HASH_{keyword.upper()} {hex(djb2(keyword))}')

    for keyword in keywords:
        print(f'case KEYWORD_HASH_{keyword.upper()}: \n'
              f'\tif (strcmp("{keyword}", buffer) == 0) {{\n '
              f'\t\tcreateSimpleToken(token, ctx, KEYWORD_{keyword.upper()});\n'
              f'\t\treturn true;\n\t}}\n'
              f'\tbreak;\n')


if __name__ == "__main__":
    main()
