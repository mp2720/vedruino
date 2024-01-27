def escape_str(s: str) -> str:
    s = s.replace('"', '\\"')
    s = s.replace('\n', '\\n')
    return s

