{
  lib,
  stdenv,
  cmake,
  ninja,
  qt6,
  copyDesktopItems,
  makeDesktopItem,
}: let
  metaData = builtins.fromJSON (builtins.readFile ../../version.json);
in
  stdenv.mkDerivation (finalAttrs: {
    pname = "physics-optical-assistant";
    inherit (metaData) version;

    src = ../../.;

    nativeBuildInputs = [
      cmake
      ninja
      qt6.wrapQtAppsHook
      copyDesktopItems
    ];
    buildInputs = [
      qt6.qtbase
      qt6.qtsvg
    ];

    installPhase = ''
      runHook preInstall

      install -Dm755 src/PHY104OpticalAssistant \
        $out/bin/PHY104OpticalAssistant

      cp -r $src/data $out/bin/

      runHook postInstall


    '';
    desktopItems = [
      (makeDesktopItem {
        name = finalAttrs.pname;
        desktopName = metaData.name;
        exec = "PHY104OpticalAssistant";
        categories = [
          "Education"
          "Science"
        ];
      })
    ];

    meta = {
      inherit (metaData) description;
      homepage = "https://github.com/movdy2/Physics_Optical_Assistant";
      changelog = "https://github.com/movdy2/Physics_Optical_Assistant/releases/";
      mainProgram = "PHY104OpticalAssistant";
      platforms = lib.platforms.linux;
    };
  })
