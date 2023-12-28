import re


def process_line(line, line_number, line_hex, flag, over_u16_hex):
    match = pattern.match(line)
    if match:
        current_number = int(match.group(1))
        current_hex = match.group(2)

        if current_number != 512:
            line_number = current_number - 0xBFB0
            if line_number >= 0x10000:
                if flag is False:
                    over_u16_hex = current_hex
                    flag = True
                line_number -= 0x10000
        else:
            line_number = current_number
        line_hex = current_hex

    return line_number, line_hex, flag, over_u16_hex


def generate_output_string(line_number, line_hex):
    return f'  {line_number},\t/* {line_hex} */\n'


if __name__ == "__main__":
    input_filename = 'input.txt'
    output_filename = 'output.txt'

    line_number, line_hex = 0, "0x0000"
    flag, over_u16_hex = False, "0x0000"
    count = 0

    pattern = re.compile(r'\s*(\d+),\s*/\*\s*\(\s*(0x[0-9a-fA-F]+)\s*\)\s*\*/')

    with open(input_filename, 'r') as input_file, open(output_filename, 'w') as output_file:

        for line in input_file:
            line_number, line_hex, flag, over_u16_hex = process_line(line,  line_number, line_hex, flag, over_u16_hex)
            output_file.write(generate_output_string(line_number, line_hex))
            count += 1
            print(count)

        if flag is True:
            print("over_u16_hex is", over_u16_hex)
