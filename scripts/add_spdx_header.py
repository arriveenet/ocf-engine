import os

# 設定
TARGET_EXTENSIONS = ('.cpp', '.hpp', '.h', '.cc', '.cxx')
SPDX_HEADER = "// SPDX-License-Identifier: MIT\n"

def add_spdx_header(directory):
    for root, dirs, files in os.walk(directory):
        for filename in files:
            if filename.endswith(TARGET_EXTENSIONS):
                file_path = os.path.join(root, filename)
                
                with open(file_path, 'r', encoding='utf-8') as f:
                    content = f.readlines()

                # ファイルが空でないか、かつ1行目にすでに識別子がないかチェック
                has_spdx = any("SPDX-License-Identifier" in line for line in content[:2])

                if not has_spdx:
                    print(f"Adding SPDX to: {file_path}")
                    # 先頭にヘッダーを追加して書き込み
                    with open(file_path, 'w', encoding='utf-8') as f:
                        f.write(SPDX_HEADER)
                        # 元のコンテンツが空でない場合、改行を挟むなどの調整が可能
                        f.writelines(content)
                else:
                    print(f"Skipping (already exists): {file_path}")

if __name__ == "__main__":
    # スクリプトを実行するディレクトリを指定（現在のディレクトリの場合は '.'）
    target_dir = input("対象ディレクトリのパスを入力してください (デフォルト '.'): ") or "."
    add_spdx_header(target_dir)
