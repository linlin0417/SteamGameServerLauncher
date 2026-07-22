; ============================================================================
; Steam Game Server Launcher - NSIS 安裝腳本
; 版權所有 (c) 2026 Yukimura Saya & The Dream Studio
; ============================================================================

; --- 基本設定 ---
Unicode true
!include "MUI2.nsh"
!include "StrFunc.nsh"
${StrStr}

!ifndef VERSION
    !define VERSION "1.3.0"
!endif

!define APP_NAME "Steam Game Server Launcher"
!define APP_EXE "SteamGameServerLauncher.exe"
!define PUBLISHER "Yukimura Saya & The Dream Studio"
!define UNINSTALL_REG_KEY "Software\Microsoft\Windows\CurrentVersion\Uninstall\SGSL"

Name "${APP_NAME}"
OutFile "SteamGameServerLauncher-${VERSION}-Setup.exe"
InstallDir "$LOCALAPPDATA\SteamGameServerLauncher"
RequestExecutionLevel user
SetCompressor /SOLID lzma

; --- 版本資訊 (檔案屬性) ---
VIProductVersion "${VERSION}.0"
VIAddVersionKey "ProductName" "${APP_NAME}"
VIAddVersionKey "CompanyName" "${PUBLISHER}"
VIAddVersionKey "LegalCopyright" "Copyright (c) 2026 ${PUBLISHER}"
VIAddVersionKey "FileDescription" "${APP_NAME} Installer"
VIAddVersionKey "FileVersion" "${VERSION}"
VIAddVersionKey "ProductVersion" "${VERSION}"

; --- MUI 設定 ---
!define MUI_ICON "build_output\SGSL_icon.ico"
!define MUI_UNICON "build_output\SGSL_icon.ico"
!define MUI_ABORTWARNING
!define MUI_LANGDLL_ALLLANGUAGES

; --- 安裝頁面順序 ---
!insertmacro MUI_PAGE_WELCOME

; 授權頁面 1: EULA
!define MUI_LICENSEPAGE_TEXT_TOP "$(LicenseHeaderEULA)"
!insertmacro MUI_PAGE_LICENSE "$(LicenseEULA)"

; 授權頁面 2: Qt 授權條款
!define MUI_LICENSEPAGE_TEXT_TOP "$(LicenseHeaderQT)"
!insertmacro MUI_PAGE_LICENSE "$(LicenseQT)"

; 授權頁面 3: 資料收集協定
!define MUI_LICENSEPAGE_TEXT_TOP "$(LicenseHeaderData)"
!insertmacro MUI_PAGE_LICENSE "$(LicenseDataCollection)"

; 目錄選擇頁面 (含 UAC 檢查)
!define MUI_PAGE_CUSTOMFUNCTION_LEAVE DirLeaveCheck
!insertmacro MUI_PAGE_DIRECTORY

!insertmacro MUI_PAGE_INSTFILES

; 完成頁面 (可勾選立即啟動)
!define MUI_FINISHPAGE_RUN "$INSTDIR\${APP_EXE}"
!define MUI_FINISHPAGE_RUN_TEXT "$(FinishRunText)"
!insertmacro MUI_PAGE_FINISH

; --- 解除安裝頁面 ---
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; --- 語言定義 (繁體中文為預設) ---
!insertmacro MUI_LANGUAGE "TradChinese"
!insertmacro MUI_LANGUAGE "English"
!insertmacro MUI_LANGUAGE "Japanese"
!insertmacro MUI_LANGUAGE "SpanishInternational"
!insertmacro MUI_LANGUAGE "French"
!insertmacro MUI_LANGUAGE "German"

; ============================================================================
; 授權檔案對應 (繁中版 / 英文版)
; 非繁體中文的語言一律使用英文版授權
; ============================================================================

; --- EULA ---
LicenseLangString LicenseEULA ${LANG_TRADCHINESE}          "licenses\EULA_zh.txt"
LicenseLangString LicenseEULA ${LANG_ENGLISH}              "licenses\EULA_en.txt"
LicenseLangString LicenseEULA ${LANG_JAPANESE}             "licenses\EULA_en.txt"
LicenseLangString LicenseEULA ${LANG_SPANISHINTERNATIONAL} "licenses\EULA_en.txt"
LicenseLangString LicenseEULA ${LANG_FRENCH}               "licenses\EULA_en.txt"
LicenseLangString LicenseEULA ${LANG_GERMAN}               "licenses\EULA_en.txt"

; --- Qt License ---
LicenseLangString LicenseQT ${LANG_TRADCHINESE}          "licenses\QT_License_zh.txt"
LicenseLangString LicenseQT ${LANG_ENGLISH}              "licenses\QT_License_en.txt"
LicenseLangString LicenseQT ${LANG_JAPANESE}             "licenses\QT_License_en.txt"
LicenseLangString LicenseQT ${LANG_SPANISHINTERNATIONAL} "licenses\QT_License_en.txt"
LicenseLangString LicenseQT ${LANG_FRENCH}               "licenses\QT_License_en.txt"
LicenseLangString LicenseQT ${LANG_GERMAN}               "licenses\QT_License_en.txt"

; --- Data Collection ---
LicenseLangString LicenseDataCollection ${LANG_TRADCHINESE}          "licenses\Data_Collection_zh.txt"
LicenseLangString LicenseDataCollection ${LANG_ENGLISH}              "licenses\Data_Collection_en.txt"
LicenseLangString LicenseDataCollection ${LANG_JAPANESE}             "licenses\Data_Collection_en.txt"
LicenseLangString LicenseDataCollection ${LANG_SPANISHINTERNATIONAL} "licenses\Data_Collection_en.txt"
LicenseLangString LicenseDataCollection ${LANG_FRENCH}               "licenses\Data_Collection_en.txt"
LicenseLangString LicenseDataCollection ${LANG_GERMAN}               "licenses\Data_Collection_en.txt"

; ============================================================================
; 多語言字串
; ============================================================================

; --- 授權頁面標題 ---
LangString LicenseHeaderEULA ${LANG_TRADCHINESE}          "最終使用者授權合約 (EULA)"
LangString LicenseHeaderEULA ${LANG_ENGLISH}              "End User License Agreement (EULA)"
LangString LicenseHeaderEULA ${LANG_JAPANESE}             "End User License Agreement (EULA)"
LangString LicenseHeaderEULA ${LANG_SPANISHINTERNATIONAL} "Acuerdo de licencia de usuario final (EULA)"
LangString LicenseHeaderEULA ${LANG_FRENCH}               "Contrat de licence utilisateur final (CLUF)"
LangString LicenseHeaderEULA ${LANG_GERMAN}               "Endbenutzer-Lizenzvereinbarung (EULA)"

LangString LicenseHeaderQT ${LANG_TRADCHINESE}          "Qt 授權條款"
LangString LicenseHeaderQT ${LANG_ENGLISH}              "Qt License Notice"
LangString LicenseHeaderQT ${LANG_JAPANESE}             "Qt License Notice"
LangString LicenseHeaderQT ${LANG_SPANISHINTERNATIONAL} "Aviso de licencia de Qt"
LangString LicenseHeaderQT ${LANG_FRENCH}               "Avis de licence Qt"
LangString LicenseHeaderQT ${LANG_GERMAN}               "Qt-Lizenzhinweis"

LangString LicenseHeaderData ${LANG_TRADCHINESE}          "資料收集協定"
LangString LicenseHeaderData ${LANG_ENGLISH}              "Data Collection Agreement"
LangString LicenseHeaderData ${LANG_JAPANESE}             "Data Collection Agreement"
LangString LicenseHeaderData ${LANG_SPANISHINTERNATIONAL} "Acuerdo de recopilacion de datos"
LangString LicenseHeaderData ${LANG_FRENCH}               "Accord de collecte de donnees"
LangString LicenseHeaderData ${LANG_GERMAN}               "Datenerfassungsvereinbarung"

; --- UAC 警告訊息 ---
LangString UACWarningMsg ${LANG_TRADCHINESE}          "若安裝到此區域，每次啟動時須進行 UAC 驗證。是否繼續?"
LangString UACWarningMsg ${LANG_ENGLISH}              "Installing to this directory requires UAC verification on every launch. Continue?"
LangString UACWarningMsg ${LANG_JAPANESE}             "このディレクトリにインストールすると、起動のたびにUAC認証が必要になります。続行しますか?"
LangString UACWarningMsg ${LANG_SPANISHINTERNATIONAL} "Instalar en este directorio requiere verificacion UAC en cada inicio. Continuar?"
LangString UACWarningMsg ${LANG_FRENCH}               "L'installation dans ce repertoire necessite une verification UAC a chaque lancement. Continuer?"
LangString UACWarningMsg ${LANG_GERMAN}               "Die Installation in diesem Verzeichnis erfordert bei jedem Start eine UAC-Verifizierung. Fortfahren?"

; --- 完成頁面 ---
LangString FinishRunText ${LANG_TRADCHINESE}          "立即啟動 ${APP_NAME}"
LangString FinishRunText ${LANG_ENGLISH}              "Launch ${APP_NAME}"
LangString FinishRunText ${LANG_JAPANESE}             "${APP_NAME} を起動する"
LangString FinishRunText ${LANG_SPANISHINTERNATIONAL} "Iniciar ${APP_NAME}"
LangString FinishRunText ${LANG_FRENCH}               "Lancer ${APP_NAME}"
LangString FinishRunText ${LANG_GERMAN}               "${APP_NAME} starten"

; ============================================================================
; 安裝路徑 UAC 檢查函式
; ============================================================================
Function DirLeaveCheck
    ; 檢查 $PROGRAMFILES (通常為 C:\Program Files)
    ${StrStr} $0 "$INSTDIR" "$PROGRAMFILES"
    StrCmp $0 "" check_programfiles64 show_warning

check_programfiles64:
    ; 檢查 $PROGRAMFILES64 (64 位元 Program Files)
    ${StrStr} $0 "$INSTDIR" "$PROGRAMFILES64"
    StrCmp $0 "" check_programfiles32 show_warning

check_programfiles32:
    ; 檢查 32 位元 Program Files (x86) 路徑
    ${StrStr} $0 "$INSTDIR" "$PROGRAMFILES32"
    StrCmp $0 "" no_warning show_warning

show_warning:
    MessageBox MB_YESNO|MB_ICONEXCLAMATION \
        "$(UACWarningMsg)" \
        IDYES no_warning
    Abort

no_warning:
FunctionEnd

; ============================================================================
; .onInit - 語言選擇對話框
; ============================================================================
Function .onInit
    !insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

; ============================================================================
; 安裝區段
; ============================================================================
Section "MainSection" SEC_MAIN
    SetOutPath "$INSTDIR"
    File /r "build_output\*.*"

    ; --- 建立桌面捷徑 ---
    CreateShortcut "$DESKTOP\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" \
        "$INSTDIR\SGSL_icon.ico" 0

    ; --- 建立開始選單捷徑 ---
    CreateDirectory "$SMPROGRAMS\${APP_NAME}"
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk" \
        "$INSTDIR\${APP_EXE}" "" \
        "$INSTDIR\SGSL_icon.ico" 0
    CreateShortcut "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk" \
        "$INSTDIR\Uninstall.exe"

    ; --- 寫入解除安裝程式 ---
    WriteUninstaller "$INSTDIR\Uninstall.exe"

    ; --- 寫入「新增/移除程式」登錄檔 ---
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "DisplayName" "${APP_NAME}"
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "UninstallString" '"$INSTDIR\Uninstall.exe"'
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "InstallLocation" "$INSTDIR"
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "Publisher" "${PUBLISHER}"
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "DisplayIcon" "$INSTDIR\SGSL_icon.ico"
    WriteRegStr HKCU "${UNINSTALL_REG_KEY}" "DisplayVersion" "${VERSION}"
    WriteRegDWORD HKCU "${UNINSTALL_REG_KEY}" "NoModify" 1
    WriteRegDWORD HKCU "${UNINSTALL_REG_KEY}" "NoRepair" 1
SectionEnd

; ============================================================================
; 解除安裝區段
; ============================================================================
Section "Uninstall"
    ; 移除檔案 (遞迴刪除安裝目錄)
    RMDir /r "$INSTDIR"

    ; 移除桌面捷徑
    Delete "$DESKTOP\${APP_NAME}.lnk"

    ; 移除開始選單捷徑
    Delete "$SMPROGRAMS\${APP_NAME}\${APP_NAME}.lnk"
    Delete "$SMPROGRAMS\${APP_NAME}\Uninstall.lnk"
    RMDir "$SMPROGRAMS\${APP_NAME}"

    ; 移除登錄檔
    DeleteRegKey HKCU "${UNINSTALL_REG_KEY}"
SectionEnd
