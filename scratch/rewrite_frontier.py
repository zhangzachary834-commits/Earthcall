with open("docs/architecture/law/LAW_EXECUTION_FRONTIER.md", "r") as f:
    content = f.read()

# Replace the reply with an updated roadmap
content = content.split("--- \n\n# Reply —")[0]
content = content.split("---\n\n# Reply")[0]

content = content.strip() + "\n"
with open("docs/architecture/law/LAW_EXECUTION_FRONTIER.md", "w") as f:
    f.write(content)

