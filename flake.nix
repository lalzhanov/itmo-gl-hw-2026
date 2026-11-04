{
  inputs = {
    nixpkgs.url = "github:nixos/nixpkgs/nixpkgs-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };

  outputs =
    {
      nixpkgs,
      flake-utils,
      ...
    }:
    flake-utils.lib.eachDefaultSystem (
      system:
      let
        pkgs = import nixpkgs { inherit system; };

        packages = with pkgs; [
          cmake
          ninja
          pkg-config
          qt6.qtbase
          libGL
          mesa

          glsl_analyzer
          clang-tools
        ];
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = packages;

          shellHook = ''
            export CMAKE_EXPORT_COMPILE_COMMANDS=ON
            export CMAKE_PREFIX_PATH="${pkgs.qt6.qtbase}:$CMAKE_PREFIX_PATH"
            export QT_PLUGIN_PATH="${pkgs.qt6.qtbase}/lib/qt-6/plugins:$QT_PLUGIN_PATH"
          '';
        };
      }
    );
}
