#!/bin/bash
# ============================================================================
# bk.sh — 依据 git status 自动备份"有用文件"（代码/文档/脚本）
# ----------------------------------------------------------------------------
# 思路：以 git status 为基准收集所有改动（已修改 tracked + 未跟踪新增），
#       再用过滤规则剔除"无用文件"（中间文件 .o/.d、日志 .log/nohup.out、
#       数据库 *.db*、编译产物 obj/、edge 运行时 shpc/、二进制可执行、
#       缓存 __pycache__、.qwen/ 工具目录 等）。
#       只把代码/文档/脚本打包进 <时间戳>.tar.gz，便于接收方 tar 解压覆盖。
# 用法：
#   ./bk.sh                 # 自动打包到 patch_<时间戳>.tar.gz
#   ./bk.sh my.patch.tar.gz # 指定输出文件名
# ============================================================================
set -u
cd "$(dirname "$0")"

OUTPUT_FILE="${1:-patch_$(date +%Y%m%d_%H%M%S).tar.gz}"

# 若需手动补充 git status 未覆盖的文件（如尚未 add 的有用目录），追加到此处：
EXTRA_FILES=(
    # "some/useful/dir/"
)

# ----------------------------------------------------------------------------
# is_useful <path> : 判断路径是否值得备份。返回 0=备份，1=排除
# ----------------------------------------------------------------------------
is_useful() {
    local f="$1"

    # 1) 排除整类"不备份目录段"
    case "$f" in
        */.git/*|*/.git)                return 1 ;;   # git 内部
        */.qwen/*|*/.qwen)              return 1 ;;   # Qwen 会话/工具目录
        */__pycache__/*|*/__pycache__)  return 1 ;;   # python 字节码缓存
        */obj/*|*/obj)                  return 1 ;;   # C++ 编译中间产物
        */shpc/*|*/shpc)                return 1 ;;   # edge 运行时生成的 frpc 配置目录
    esac

    # 2) 排除"无用扩展名"文件
    case "$f" in
        *.o|*.d|*.so|*.a|*.lo)          return 1 ;;   # 编译中间/库
        *.log|*.out)                    return 1 ;;   # 日志
        *.db|*.db-shm|*.db-wal|*.sqlite|*.sqlite3) return 1 ;;  # 数据库
        *.pyc|*.pyo)                    return 1 ;;   # python 字节码
        *.tar.gz|*.tgz|*.zip|*.gz|*.bz2|*.xz) return 1 ;;       # 压缩包（备份产物）
        *.png|*.jpg|*.jpeg|*.gif)       return 1 ;;   # 图片（非必要）
    esac

    # 3) 按"文件名"排除特定产物
    local name="${f##*/}"
    case "$name" in
        *-linux)                        return 1 ;;   # 编译出的 ELF 可执行（如 touch_edge-linux）
        nohup.out)                      return 1 ;;   # 后台运行日志
        core|core.*)                    return 1 ;;   # 崩溃转储
        *~|*.swp|*.tmp)                 return 1 ;;   # 编辑器/临时文件
    esac

    # 4) 目录（以 / 结尾）整体排除——git status 未跟踪目录按无用处理；
    #    若有用目录需备份，请 git add 或加入 EXTRA_FILES
    case "$f" in
        */) return 1 ;;
    esac

    return 0
}

# ----------------------------------------------------------------------------
# 收集文件清单
# ----------------------------------------------------------------------------
declare -a FILES
declare -a SKIPPED
declare -a SEEN=()

add_file() {
    local f="$1"
    # 去重
    for s in "${SEEN[@]:-}"; do [ "$s" = "$f" ] && return; done
    SEEN+=("$f")
    if is_useful "$f"; then
        if [ -e "$f" ]; then
            FILES+=("$f")
        else
            SKIPPED+=("$f  (路径不存在)")
        fi
    else
        SKIPPED+=("$f")
    fi
}

# A) 遍历 git status（porcelain：前 2 字符为状态码，第 3 字符起为路径）
while IFS= read -r line; do
    [ -z "$line" ] && continue
    path="${line:3}"
    add_file "$path"
done < <(git status --porcelain)

# B) 手动补充
for f in "${EXTRA_FILES[@]:-}"; do
    [ -z "$f" ] && continue
    add_file "$f"
done

# ----------------------------------------------------------------------------
# 输出 & 打包
# ----------------------------------------------------------------------------
echo "=========================================================="
echo " 依据 git status 收集：${#FILES[@]} 个有用文件 -> ${OUTPUT_FILE}"
echo "=========================================================="
if [ ${#FILES[@]} -eq 0 ]; then
    echo "❌ 没有可备份的有用文件（git status 无改动，或全部被过滤）"
    echo "   已排除 ${#SKIPPED[@]} 项："
    printf '   - %s\n' "${SKIPPED[@]}"
    exit 1
fi

echo ""
echo "将打包："
printf '   + %s\n' "${FILES[@]}"
echo ""
echo "已排除（中间/日志/db/二进制/目录） ${#SKIPPED[@]} 项："
printf '   - %s\n' "${SKIPPED[@]}"
echo ""
echo "----- 开始打包 -----"
tar -czf "$OUTPUT_FILE" "${FILES[@]}"
rc=$?
if [ $rc -eq 0 ]; then
    echo "✅ 打包成功：$OUTPUT_FILE"
    echo "   文件数：${#FILES[@]}   大小：$(du -h "$OUTPUT_FILE" | cut -f1)"
    echo ""
    echo "接收方在目标工程根目录执行以下命令即可同步覆盖："
    echo "   tar -xzf $OUTPUT_FILE"
else
    echo "❌ 打包失败（tar 退出码 $rc），请检查上述文件路径"
    exit $rc
fi
