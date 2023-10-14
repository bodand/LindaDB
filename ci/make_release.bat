@echo off

if [%CG_TOOLCHAIN]==[clang] (
    echo:clang-cl is not used to upload releases ATM
    goto END
)

cd %BUILD_PATH%

curl ^
  -H "Accept: application/vnd.github+json" ^
  -H "Authorization: Bearer %GITHUB_TOKEN%" ^
  -H "X-GitHub-Api-Version: 2022-11-28" ^
  "https://api.github.com/repos/bodand/cg3/releases" -o releases.json

for /f "usebackq tokens=*" %%r in (`jq ".[]|select(.draft).id" releases.json`) do set "draft_id=%%r"

if NOT DEFINED draft_id goto CREAT_DRAFT

echo:Draft release found; using existing release with id %draft_id%
:WRITE_ENV

for /f "usebackq tokens=*" %%r in (`jq ".[]|select(.draft).id" releases.json`) do echo:set "GH_REL_ID=%%r" >> vars.cmd

goto END

:CREAT_DRAFT
echo:Draft release not found; creating...

jq --null-input ^
  --arg tag_name "%CIRRUS_TAG%" ^
  --arg name "cg3 release %CIRRUS_TAG%" ^
  --arg body "$(cat %CIRRUS_WORKING_DIR%/changelog/ChangeLog-%CIRRUS_TAG%.md)" ^
  "{"tag_name": $tag_name, "name": $name, "body": $body, "generate_release_notes": false, "draft": true}" ^
  >release-payload.json

curl -X POST ^
  -H "Accept: application/vnd.github+json" ^
  -H "Authorization: Bearer %GITHUB_TOKEN%" ^
  -H "X-GitHub-Api-Version: 2022-11-28" ^
  "https://api.github.com/repos/bodand/cg3/releases" ^
  -d "@release-payload.json"

curl ^
  -H "Accept: application/vnd.github+json" ^
  -H "Authorization: Bearer %GITHUB_TOKEN%" ^
  -H "X-GitHub-Api-Version: 2022-11-28" ^
  "https://api.github.com/repos/bodand/cg3/releases" -o releases.json

goto WRITE_ENV

:END
