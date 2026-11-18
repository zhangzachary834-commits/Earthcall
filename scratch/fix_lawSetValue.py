with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "r") as f:
    text = f.read()

text = text.replace('lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile, "FileRead");', 'lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile);')
text = text.replace('lawSetValue(subject, PropertyPath::parse("@file-channel.file.read"), PropertyValue(true), "FileRead");', 'lawSetValue(subject, PropertyPath::parse("@file-channel.file.read"), PropertyValue(true));')
text = text.replace('lawSetValue(subject, destPath, content, "FileRead");', 'lawSetValue(subject, destPath, content);')

text = text.replace('lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile, "FileWrite");', 'lawSetValue(subject, PropertyPath::parse("@file-channel.file.path"), pvFile);')
text = text.replace('lawSetValue(subject, PropertyPath::parse("@file-channel.file.content"), pvContent, "FileWrite");', 'lawSetValue(subject, PropertyPath::parse("@file-channel.file.content"), pvContent);')
text = text.replace('lawSetValue(subject, PropertyPath::parse("@file-channel.file.write"), PropertyValue(true), "FileWrite");', 'lawSetValue(subject, PropertyPath::parse("@file-channel.file.write"), PropertyValue(true));')

text = text.replace('lawSetValue(subject, PropertyPath::parse("@codec.codec.input"), cv, "CodecTransform");', 'lawSetValue(subject, PropertyPath::parse("@codec.codec.input"), cv);')
text = text.replace('lawSetValue(subject, destPath, result, "CodecTransform");', 'lawSetValue(subject, destPath, result);')

with open("src/ZonesOfEarth/AuthorsOfLaw/ActionModel.cpp", "w") as f:
    f.write(text)

