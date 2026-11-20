set -u
T0=$(date +%s); MK0=$(cat "${CLOUD_ENV_MARKER:-/opt/cloud-env-setup}" 2>/dev/null); FT0=$(date -u +%Y-%m-%dT%H:%M:%SZ)   # маркер от поля среды и время его снятия — ДО cloud-env-sync (доустановка пишет свой): at наблюдения = время MK0
command -v cloud-env-sync >/dev/null && cloud-env-sync > /tmp/sync.log 2>&1; SYNC_RC=$?; command -v cloud-env-sync >/dev/null || SYNC_RC=0   # нет команды — не ошибка
for f in /etc/profile.d/*.sh; do [ -r "$f" ] && { set +u; . "$f"; set -u; }; done   # переменные среды (CAD_FREECADCMD и т. п.): lead-mockups 09.11
SYNC_WHAT=; if [ "$SYNC_RC" -eq 0 ]; then { echo "== fw-prepare"; fw-prepare; } >> /tmp/sync.log 2>&1; EP=$?; [ "$EP" -ne 0 ] && { SYNC_RC=$EP; SYNC_WHAT="fw-prepare упал"; }; fi
rm -f "${CLOUD_FIELD_JSON:-/tmp/field.json}"
FKC='import base64,os
from cryptography.hazmat.primitives.asymmetric import ed25519 as ed
from cryptography.hazmat.primitives import serialization as s
k=ed.Ed25519PrivateKey.from_private_bytes(base64.b64decode(os.environ['"'"'REMOTECHECK_CLOUD_KEY'"'"'].strip()))
p=base64.b64encode(k.public_key().public_bytes(s.Encoding.Raw,s.PublicFormat.Raw)).decode()
print({'"'"'ZJYt1q8gAljdZVowmnXbkIXkmEGiXQNe/S8dBd/ii+U='"'"': '"'"'cloud-pc2'"'"', '"'"'BUsZ8u9LhyS6lW8hN/22oDLgw5Krj9bRZt0VO5XtlKI='"'"': '"'"'cloud-pc1'"'"', '"'"'SYkbusByXSgVUecXjf6MERPe6SKtS+RazMwlHAuPlP4='"'"': '"'"'cloud-lap'"'"', '"'"'n4KyA3S1UKmlAPW03O5lwSESiTYg8/PIlLsnFh8c2c4='"'"': '"'"'cloud-mini'"'"', '"'"'KFe8d4/asTu/vm9rADiabWXTD8UOowOA6fK+DKhHbyk='"'"': '"'"'cloud-n150'"'"', '"'"'gpBl2IFa3Hz6zriaBl2tblwK2DHC+Se8c154SgRBFwU='"'"': '"'"'cloud-mac'"'"'}.get(p,'"'"''"'"'))
'; FM=; for P in python3 /opt/*/venv/bin/python /usr/bin/python3; do [ -x "$(command -v $P 2>/dev/null)" ] || continue; FM=$($P -c "$FKC" 2>/dev/null) && [ -n "$FM" ] && break; FM=; done
FACC=${FM#cloud-}; OWN=
case "$FACC" in
  pc2) OWN=;;
  pc1) OWN=envs/firmware;;
  mini) OWN=slots/slot-6;;
esac
FS=${MK0%% *}
FA=$OWN; FK=unknown
if [ -n "$MK0" ]; then case "$FS" in *[!0-9a-f]*) FS=;; *)
  FK=full-text
  case "$FS" in
    f87568bd3d34fbf8642390b90112af0fed3cb9ddd467b208335b2f69eab768bd) FA=envs/firmware; FK=mirror-line;;
    fa06a1e509a9a3527dcdd86c841acefa6a1370be5152ff7229f6e827f3513489) FA=slots/slot-6; FK=mirror-line;;
  esac;;
esac; else FS=; fi
one() { printf '{"acct":"%s","addr":"%s","sha":"%s","kind":"%s","at":"%s"}' "$FACC" "$1" "$2" "$3" "$FT0"; }
if [ -z "$FACC" ] || [ -z "$FT0" ]; then :   # нет учётки запуска или времени MK0 — наблюдения нет, а не чужое имя
elif [ -n "$FA" ]; then one "$FA" "$FS" "$FK" > "${CLOUD_FIELD_JSON:-/tmp/field.json}"
else S=; { printf "["; for A in envs/firmware slots/slot-6; do printf "$S"; one "$A" "" unknown; S=,; done; printf "]"; } > "${CLOUD_FIELD_JSON:-/tmp/field.json}"
fi
if [ "$SYNC_RC" -eq 0 ]; then MKP="${CLOUD_ENV_MARKER:-/opt/cloud-env-setup}"; MK=$(cat "$MKP" 2>/dev/null); MW=; SYNC_NOTE=; VER=$(cat "${CLOUD_ENV_VERSION:-/opt/cloud-env-version}" 2>/dev/null); VOK=
  [ -z "$VER" ] || case "$VER" in "1bf22b433697"|"56e92f27469e"|"ebcb99c5d581") VOK=1;; esac
  [ -n "$MK0$MK" ] || [ -z "$VOK" ] || { SYNC_NOTE="поле setup среды firmware (учётка pc1): маркера нет, но версия окружения совпала с зеркалом — принято (файл маркера из сборки не доехал)"; echo "$SYNC_NOTE" >> /tmp/sync.log; }
  [ -n "$SYNC_NOTE" ] || case "$MK0" in "f87568bd3d34fbf8642390b90112af0fed3cb9ddd467b208335b2f69eab768bd "*|"e347f340bcf40c548bb616f5e65bed7c75ef3161d30df2368696df92380c49ca "*|"fa06a1e509a9a3527dcdd86c841acefa6a1370be5152ff7229f6e827f3513489 "*) ;; *) if [ -z "$MK0" ]; then [ -n "$VOK" ] || MW="нет маркера до cloud-env-sync"; else MW="sha ${MK0%% *} до cloud-env-sync"; fi;; esac
  STALE=; [ -n "$MW$SYNC_NOTE" ] || case "$MK" in "f87568bd3d34fbf8642390b90112af0fed3cb9ddd467b208335b2f69eab768bd 1bf22b433697"|"e347f340bcf40c548bb616f5e65bed7c75ef3161d30df2368696df92380c49ca 56e92f27469e"|"fa06a1e509a9a3527dcdd86c841acefa6a1370be5152ff7229f6e827f3513489 ebcb99c5d581"|"e347f340bcf40c548bb616f5e65bed7c75ef3161d30df2368696df92380c49ca 1bf22b433697") ;; *) case "${MK#* }" in "1bf22b433697"|"56e92f27469e"|"ebcb99c5d581") MW="sha ${MK%% *}, версия сценария ${MK#* } после cloud-env-sync";; *) CV=$(curl -fsS -m 20 "$(sed -n "s/^u=//p" "${CLOUD_ENV_SYNC:-/usr/local/bin/cloud-env-sync}" 2>/dev/null | head -1)/version" 2>/dev/null); if [ -n "$MK" ] && [ -n "$CV" ] && [ "${MK#* }" = "$CV" ] && [ "$VER" = "$CV" ]; then STALE=1; else MW="sha ${MK%% *}, версия сценария ${MK#* } после cloud-env-sync"; fi;; esac;; esac
  [ -z "$STALE" ] || { SYNC_RC=1; SYNC_WHAT="метка старая: собрана под версии 1bf22b433697 56e92f27469e ebcb99c5d581, на зеркале ${MK#* } — пересобери метку на нынешнем main, поле не трогать (среда firmware (учётка pc1))"; echo "$SYNC_WHAT" >> /tmp/sync.log; }
  [ -z "$MW" ] || { SYNC_RC=1; SYNC_WHAT="поле setup среды firmware (учётка pc1) расходится с render ($MW)"; echo "$SYNC_WHAT" >> /tmp/sync.log; }
fi
publish_part() {
local PE=${PUSH_ERR:-/tmp/push-$K.err}; PUSHED=; PERR=; : > "$PE"; for i in $(seq 1 20); do rm -rf /tmp/res; git worktree prune; git fetch -q origin cloud-results 2>"$PE" && git worktree add -q -f --detach /tmp/res FETCH_HEAD 2>>"$PE" || { PERR=$(tail -n1 "$PE"); sleep $((RANDOM%15+3)); continue; }
 F=/tmp/res/$D/part-$K-of-1; cp /tmp/ra.log $F.log; cp /tmp/rc $F.rc; [ -e /home/user/p.json ] && cp /home/user/p.json $F.json; [ -e /tmp/usage.json ] && cp /tmp/usage.json $F.usage.json; [ -e "${CLOUD_FIELD_JSON:-/tmp/field.json}" ] && cp "${CLOUD_FIELD_JSON:-/tmp/field.json}" $F.field.json; git -C /tmp/res add $D; git -C /tmp/res commit -qm "cloud-results: 1575583a-1-11201351 часть $K/1" >/dev/null 2>"$PE" || git -C /tmp/res diff --cached --quiet || { PERR=$(tail -n1 "$PE"); sleep $((RANDOM%15+3)); continue; }
 git -C /tmp/res push -q origin HEAD:refs/heads/cloud-results 2>"$PE" && git fetch -q origin cloud-results 2>>"$PE" && git cat-file -e FETCH_HEAD:$D/part-$K-of-1.rc 2>/dev/null && { echo "push годно"; PUSHED=1; break; }; PERR=$(tail -n1 "$PE"); sleep $((RANDOM%15+3)); done
[ -n "$PUSHED" ] || { echo "push результата НЕ годен: ${PERR:-итог не виден в cloud-results} (20 попыток)"; echo "ОТКАЗ: итог части $K не доставлен в cloud-results за 20 попыток"; cat /tmp/rc; tail -c 2000 /tmp/ra.log; return 1; }
}
tree_guard() {   # $1 — проверенная вершина (HEAD после подготовки); план b24-831
local R=/home/user/esp-swd-probe V="${1:-HEAD}" S P
S=$(git -C "$R" status --porcelain --untracked-files=all 2>&1) || S="git status упал: $S"
[ -z "$S" ] && return 0
P=$(printf '%s\n' "$S" | head -10 | cut -c4- | paste -sd' ' -)
echo "дерево грязное после прогона: $P" | tee -a /tmp/ra.log
git -C "$R" reset -q --hard "$V" && git -C "$R" clean -fdq
[ -z "$(git -C "$R" status --porcelain --untracked-files=all 2>&1)" ] || { P="$P; очистка дерева не удалась"; echo "очистка дерева не удалась" | tee -a /tmp/ra.log; }
echo "дерево грязное после прогона: $P" >> /tmp/tree-dirty
return 1
}
tree_mark() {   # запись tree_guard — в итог части: код 0 -> 1, иной код сохраняется
[ -s /tmp/tree-dirty ] || return 0
local L N; N=$(paste -sd";" - < /tmp/tree-dirty); L=$(head -1 /tmp/rc 2>/dev/null); [ -n "$L" ] || L="код 0, 0 с"
case "$L" in "код 0,"*) L="код 1,${L#код 0,}";; esac
echo "$L — $N" > /tmp/rc; cat /tmp/rc
}
rm -f /tmp/tree-dirty
prep_red() {
set +e; RC=$1; rm -f /tmp/usage.json; cd /home/user/esp-swd-probe
{ echo "среда $(cat /opt/cloud-env-version 2>/dev/null)"; [ -s /tmp/sync.log ] && { echo "== cloud-env-sync, хвост:"; tail -c 3000 /tmp/sync.log; echo "== конец cloud-env-sync"; }; } > /tmp/ra.log 2>&1
{ echo "$PREP_MSG; cmd не выполнен"; cat /tmp/prep.log; } >> /tmp/ra.log 2>&1
echo "код $RC, 0 с, с начала сессии $(( $(date +%s)-T0 )) с${PREP_MSG:+ — $PREP_MSG}" > /tmp/rc; cat /tmp/rc
tree_guard "$(git -C /home/user/esp-swd-probe rev-parse -q --verify HEAD 2>/dev/null || echo HEAD)"; tree_mark   # грязное дерево — часть красная, пути в итоге (план b24-831)
PRC=0; publish_part || PRC=$?   # отказ публикации не обрывает функцию (set -e из подготовки); DONE — у вызывающего
return $PRC
}
trap 'rc=$?; if [ -n "${FIN:-}" ]; then :; elif [ -n "${PREP_PHASE:-}" ]; then exec >&3 2>&4; PREP_PHASE=; PREP_MSG="подготовка упала, код $rc"; [ "$rc" -eq 0 ] && { rc=1; PREP_MSG="подготовка вышла exit 0 до конца"; }; prep_red $rc; FIN=1; echo DONE; exit $rc; else echo "DONE обрыв run.sh, код $rc"; fi' EXIT
export REMOTECHECK_MACHINE=cloud-pc1   # имя машины подписи = ключ учётки дежурного (lead-boost 09.11, cloud_gate.py)
MK='import base64,os
from cryptography.hazmat.primitives.asymmetric import ed25519 as ed
from cryptography.hazmat.primitives import serialization as s
k=ed.Ed25519PrivateKey.from_private_bytes(base64.b64decode(os.environ['"'"'REMOTECHECK_CLOUD_KEY'"'"'].strip()))
p=base64.b64encode(k.public_key().public_bytes(s.Encoding.Raw,s.PublicFormat.Raw)).decode()
print({'"'"'ZJYt1q8gAljdZVowmnXbkIXkmEGiXQNe/S8dBd/ii+U='"'"': '"'"'cloud-pc2'"'"', '"'"'BUsZ8u9LhyS6lW8hN/22oDLgw5Krj9bRZt0VO5XtlKI='"'"': '"'"'cloud-pc1'"'"', '"'"'SYkbusByXSgVUecXjf6MERPe6SKtS+RazMwlHAuPlP4='"'"': '"'"'cloud-lap'"'"', '"'"'n4KyA3S1UKmlAPW03O5lwSESiTYg8/PIlLsnFh8c2c4='"'"': '"'"'cloud-mini'"'"', '"'"'KFe8d4/asTu/vm9rADiabWXTD8UOowOA6fK+DKhHbyk='"'"': '"'"'cloud-n150'"'"', '"'"'gpBl2IFa3Hz6zriaBl2tblwK2DHC+Se8c154SgRBFwU='"'"': '"'"'cloud-mac'"'"'}.get(p,'"'"''"'"'))
'; M=; for P in python3 /opt/*/venv/bin/python /usr/bin/python3; do [ -x "$(command -v $P 2>/dev/null)" ] || continue; M=$($P -c "$MK" 2>/dev/null) && [ -n "$M" ] && break; M=; done; if [ -n "$M" ]; then export REMOTECHECK_MACHINE=$M; elif [ -n "${REMOTECHECK_CLOUD_KEY:-}" ]; then echo "⚠ имя машины подписи не выведено из ключа (нет cryptography ни в одном питоне) — подпись отключена" >&2; unset REMOTECHECK_MACHINE; fi
PN=0
while :; do
PN=$((PN+1))
if [ "$PN" -gt 1 ]; then
[ "$PN" -le 4 ] && [ $(( 3600-($(date +%s)-T0) )) -ge 3600 ] || break
FIN=; PREP_PHASE=; AOK=; SYNC_RC=0; SYNC_WHAT=; K=""; cd /home/user/esp-swd-probe && git reset -q --hard
rm -rf /tmp/res /tmp/art /tmp/rc /tmp/usage.json /tmp/prep.log /tmp/ra.log /home/user/p.json; git worktree prune
rm -f /tmp/tree-dirty   # Codex п. 1: пометка грязного дерева прошлой части не красит следующую
CLOUD_ARTIFACTS_DIR="${CLOUD_ARTIFACTS_DIR:-/home/user/artifacts}"; [ -d "$CLOUD_ARTIFACTS_DIR" ] && find "$CLOUD_ARTIFACTS_DIR" -mindepth 1 -delete
fi
cd /home/user/esp-swd-probe && git fetch -q origin fix/usb-serial-mac && { git fetch -q --tags origin || echo "⚠ git fetch --tags: код $?"; } && git switch -q --detach 1575583a24ad2978f809d7dcbe13a0f049005006 || { echo 'нет sha'; FIN=1; echo DONE; exit; }
git config --global user.name cloud-runner; git config --global user.email cloud-runner@local
D=runs/1575583a-1-11201351; K=""
for try in $(seq 1 40); do rm -rf /tmp/res; git worktree prune; git fetch -q origin cloud-results && git worktree add -q -f --detach /tmp/res FETCH_HEAD || { git worktree add -q -f --detach /tmp/res && git -C /tmp/res checkout -q --orphan cr-tmp && git -C /tmp/res rm -rqf . ; }
 for k in $(seq 1 1); do [ -e /tmp/res/$D/claim-$k ] || { K=$k; break; }; done
 [ -z "$K" ] && { echo "все части заняты"; FIN=1; echo DONE; exit; }
 mkdir -p /tmp/res/$D && { date -u +%T; printenv CLAUDE_CODE_REMOTE_SESSION_ID || echo -; hostname; } | paste -sd" " > /tmp/res/$D/claim-$K && git -C /tmp/res add $D && git -C /tmp/res commit -qm "claim $K" && git -C /tmp/res push -q origin HEAD:refs/heads/cloud-results && break
 K=""; sleep $((RANDOM%5+1)); done
[ -n "$K" ] || { echo "захват части не подтверждён за 40 попыток — cmd не выполнен"; FIN=1; echo DONE; exit; }
echo "часть $K/1, старт $(date -u +%T)"
if [ "${MW:-}" = "нет маркера до cloud-env-sync" ] && [ ! -e /tmp/res/$D/snap-$K ]; then for try in 1 2 3 4 5; do { date -u +%T; hostname; echo "$SYNC_WHAT"; } | paste -sd" " > /tmp/res/$D/snap-$K && git -C /tmp/res rm -q $D/claim-$K && git -C /tmp/res add $D && git -C /tmp/res commit -qm "snap $K" && git -C /tmp/res push -q origin HEAD:refs/heads/cloud-results && { echo "часть $K/1: снимок среды без маркера — захват снят, повтор новой сессией (snap-$K)"; FIN=1; echo DONE; exit 0; }; git -C /tmp/res fetch -q origin cloud-results && git -C /tmp/res reset -q --hard FETCH_HEAD; [ -e /tmp/res/$D/snap-$K ] && { echo "часть $K/1: снятие захвата уже в origin — повтор новой сессией (snap-$K)"; FIN=1; echo DONE; exit 0; }; [ -e /tmp/res/$D/claim-$K ] && [ ! -e /tmp/res/$D/part-$K-of-1.rc ] || break; done; if [ ! -e /tmp/res/$D/claim-$K ] || [ -e /tmp/res/$D/part-$K-of-1.rc ]; then echo "⚠ часть $K: захват после снимка без маркера уже не наш — итог не публикуем"; FIN=1; echo DONE; exit 0; fi; echo "⚠ захват части $K после снимка без маркера не снят — красный итог"; fi
PREP_MSG=; PREP_RC=0; if [ "$SYNC_RC" -ne 0 ]; then PREP_RC=$SYNC_RC; PREP_MSG="${SYNC_WHAT:-cloud-env-sync упал}, код $SYNC_RC"; : > /tmp/prep.log; else
exec 3>&1 4>&2; PREP_PHASE=1   # fd 3/4: exit в подготовке оставляет её перенаправление в силе, trap вернёт вывод
{
{ S="${CLOUD_PLATFORM_SRC:-/home/user/platform}"; P=$(sed -n 's/^url *= *"\(.*\)" *$/\1/p' tool/platform.pin 2>/dev/null | head -n 1); P=${P%.git}; [ -z "$P" ] || [ ! -d "$S/.git" ] || { U=$(git -C "$S" remote get-url origin) && for A in "$P.git" "$P"; do git config --global --add "url.$U.insteadOf" "$A" || exit 1; done; }; } && { [ ! -f tool/pin/platform_pin.py ] || python3 tool/pin/platform_pin.py sync; } && {
fw-prepare && git -C /home/user/esp-swd-probe submodule update --init components/corsacOTA
}
} > /tmp/prep.log 2>&1; PREP_RC=$?
PREP_PHASE=
cat /tmp/prep.log; [ "$PREP_RC" -ne 0 ] && PREP_MSG="подготовка упала, код $PREP_RC"
fi
[ "$PREP_RC" -eq 0 ] || { prep_red $PREP_RC; [ "$PRC" -eq 0 ] && [ "$SYNC_RC" -eq 0 ] && continue; FIN=1; echo DONE; exit $PRC; }
VERIFIED=$(git -C /home/user/esp-swd-probe rev-parse -q --verify HEAD 2>/dev/null || echo HEAD)
set +e; set +o errexit
{ echo "среда $(cat /opt/cloud-env-version 2>/dev/null)"; [ -s /tmp/sync.log ] && { echo "== cloud-env-sync, хвост:"; tail -c 3000 /tmp/sync.log; echo "== конец cloud-env-sync"; }; } > /tmp/ra.log 2>&1
CLOUD_ARTIFACTS_DIR="${CLOUD_ARTIFACTS_DIR:-/home/user/artifacts}"; AE=$(mkdir -p "$CLOUD_ARTIFACTS_DIR" 2>&1 && chmod 1777 "$CLOUD_ARTIFACTS_DIR" 2>&1) || { PREP_MSG="artifacts: ОТКАЗ подготовки $CLOUD_ARTIFACTS_DIR: $AE"; echo "$PREP_MSG" | tee -a /tmp/ra.log; prep_red 1; [ "$PRC" -eq 0 ] && continue; FIN=1; echo DONE; exit $PRC; }; export CLOUD_ARTIFACTS_DIR
rm -f /home/user/p.json /tmp/part-timeout.flag; s=$(date +%s); ( cd /home/user/esp-swd-probe && sh tests/host/run.sh && { ls -d /opt/esp* $HOME/esp* 2>/dev/null; echo IDF_PATH=$IDF_PATH; } && . "${IDF_PATH:-$HOME/esp/esp-idf}/export.sh" >/dev/null && rm -f sdkconfig && idf.py set-target esp32s3 && idf.py build && sha256sum build/bootloader/bootloader.bin build/partition_table/partition-table.bin build/wireless_esp_dap.bin && ls -l build/bootloader/bootloader.bin build/partition_table/partition-table.bin build/wireless_esp_dap.bin ) >> /tmp/ra.log 2>&1 & CP=$!; ( sleep 3600; : > /tmp/part-timeout.flag; KIDS=$(pgrep -P $CP); kill -STOP $CP; kill -TERM $CP; for p in $KIDS; do pkill -TERM -P $p; kill -TERM $p; done; kill -CONT $CP; sleep 10; for p in $KIDS $CP; do pkill -KILL -P $p; kill -KILL $p; done ) >/dev/null 2>&1 & WD=$!; wait $CP; RC=$?; kill -STOP $WD 2>/dev/null; SP=$(pgrep -P $WD); kill -KILL $WD $SP 2>/dev/null; wait $WD 2>/dev/null; if [ -e /tmp/part-timeout.flag ]; then RC=124; fi
T=; [ -e /tmp/part-timeout.flag ] && { T=" — обрыв по времени: предел части part_timeout_s, 3600 с"; echo "обрыв по времени: предел части part_timeout_s, 3600 с" >> /tmp/ra.log; }; echo "код $RC, $(( $(date +%s)-s )) с, с начала сессии $(( $(date +%s)-T0 )) с$T" > /tmp/rc; cat /tmp/rc
cd /home/user/esp-swd-probe   # подготовка могла сменить каталог; git ниже — из репозитория
tree_guard "$VERIFIED"; tree_mark   # грязное дерево — часть красная, пути в итоге (план b24-831)
if [ -d "$CLOUD_ARTIFACTS_DIR" ] && [ -n "$(ls -A "$CLOUD_ARTIFACTS_DIR")" ]; then AB=cloud-artifacts/1575583a-1-11201351-$K; SZ=$(du -sm "$CLOUD_ARTIFACTS_DIR" | cut -f1); if [ "$SZ" -gt 300 ]; then echo "artifacts: $SZ МБ больше 300 — не отправлены" | tee -a /tmp/ra.log; else rm -rf /tmp/art; git worktree prune; git worktree add -q -f --detach /tmp/art && git -C /tmp/art checkout -q --orphan art-tmp && { git -C /tmp/art rm -rqf . >/dev/null 2>&1; cp -r "$CLOUD_ARTIFACTS_DIR"/. /tmp/art/ && git -C /tmp/art add -A && git -C /tmp/art commit -qm "artifacts 1575583a-1-11201351 часть $K"; } && for i in 1 2 3 4 5; do git -C /tmp/art push -q origin HEAD:refs/heads/$AB && { AOK=1; break; }; sleep 5; done; [ -n "${AOK:-}" ] && echo "artifacts: ветка $AB, $SZ МБ" | tee -a /tmp/ra.log || echo "artifacts: ОТКАЗ push в $AB ($SZ МБ)" | tee -a /tmp/ra.log; fi; elif [ -d "$CLOUD_ARTIFACTS_DIR" ]; then echo "artifacts: пусто" | tee -a /tmp/ra.log; fi
U=/tmp/usage.json; python3 /home/user/esp-swd-probe/tool/lead/tokusage.py --root "$HOME/.claude/projects" --sum --from "$(date -u -d @$T0 +%Y-%m-%dT%H:%M:%SZ)" --to "$(date -u +%Y-%m-%dT%H:%M:%SZ)" > $U 2>/dev/null || printf '{"usage": {"error": "tokusage.py не отработал, код %s"}}\n' $? > $U
publish_part; PRC=$?
FIN=1; [ "$PRC" -eq 0 ] || { echo DONE; exit $PRC; }   # итог не доставлен — run.sh кончается кодом ≠ 0
done
FIN=1; echo DONE; exit 0
