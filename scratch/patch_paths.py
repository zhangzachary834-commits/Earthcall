with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

text = text.replace('PropertyPath::parse("file-channel.path")', 'PropertyPath::parse("@file-channel.file.path")')
text = text.replace('PropertyPath::parse("file-channel.readTrigger")', 'PropertyPath::parse("@file-channel.file.read")') # wait, what was readTrigger called?
text = text.replace('PropertyPath::parse("file-channel.content")', 'PropertyPath::parse("@file-channel.file.content")')
text = text.replace('PropertyPath::parse("file-channel.writeTrigger")', 'PropertyPath::parse("@file-channel.file.write")') # wait, what was writeTrigger called?

text = text.replace('PropertyPath::parse("codec.input")', 'PropertyPath::parse("@codec.codec.input")')
text = text.replace('PropertyPath::parse("codec." + op)', 'PropertyPath::parse("@codec.codec." + op)')

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)
