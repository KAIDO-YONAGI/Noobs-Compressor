#!/bin/bash

# 创建恢复目录
RECOVERY_DIR="Git_Recovery"
mkdir -p "$RECOVERY_DIR"

echo "开始恢复 dangling 对象..."

# 恢复 dangling blob (文件内容)
git fsck --lost-found | grep "dangling blob" | while read -r _ _ hash; do
    echo "恢复文件: $hash"
    git cat-file -p "$hash" > "$RECOVERY_DIR/file_$hash"
done

# 恢复 dangling tree (目录结构)
git fsck --lost-found | grep "dangling tree" | while read -r _ _ hash; do
    echo "恢复目录结构: $hash"
    TREE_DIR="$RECOVERY_DIR/tree_$hash"
    mkdir -p "$TREE_DIR"
    (cd "$TREE_DIR" && git read-tree "$hash" && git checkout-index -af)
done

# 恢复 dangling commit (提交内容)
git fsck --lost-found | grep "dangling commit" | while read -r _ _ hash; do
    echo "恢复提交: $hash"
    git show "$hash" > "$RECOVERY_DIR/commit_$hash.txt"
done

echo "✅ 恢复完成！文件保存在 $RECOVERY_DIR 目录中"
