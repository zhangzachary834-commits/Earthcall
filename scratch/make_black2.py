from PIL import Image
img = Image.new("RGBA", (2, 2), "black")
img.save("scratch/test2.png")
with open("scratch/test2.png", "rb") as f:
    data = f.read()
c_array = ", ".join([f"0x{x:02x}" for x in data])
print(f"const unsigned char png_data[] = {{ {c_array} }};")
