import sys
# 2x2 black PNG hex from standard transparent/black
png_hex = "89504E470D0A1A0A0000000D494844520000000200000002080600000072B60D240000000B49444154085B636000020000050001099232980000000049454E44AE426082"
b = bytes.fromhex(png_hex)

c_array = ", ".join([f"0x{x:02x}" for x in b])
print(f"const unsigned char png_data[] = {{ {c_array} }};")
