"""MkDocs hook that links SQL examples to the playground.

Each SQL code block on a page under examples/ gets a "Run in the playground"
link. The link carries every SQL block on the page up to and including the
current one, so blocks that depend on earlier setup statements still work when
the playground executes the combined script.
"""

import re
import urllib.parse

PLAYGROUND_URL = "https://cogitatortech.github.io/onager/playground/"

SQL_BLOCK = re.compile(r"^```sql\n(.*?)^```$", re.DOTALL | re.MULTILINE)


def on_page_markdown(markdown, page, config, files):
    if not page.file.src_uri.startswith("examples/"):
        return markdown

    blocks_so_far = []

    def add_link(match):
        blocks_so_far.append(match.group(1).strip())
        script = "\n\n".join(blocks_so_far)
        href = PLAYGROUND_URL + "#sql=" + urllib.parse.quote(script, safe="")
        return f"{match.group(0)}\n\n[Run in the playground]({href})\n"

    return SQL_BLOCK.sub(add_link, markdown)
