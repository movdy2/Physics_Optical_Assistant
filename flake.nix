{
  description = "Physics Optical Assistant.";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {nixpkgs, ...}: let
    inherit (nixpkgs.lib) genAttrs;
    systems = [
      "x86_64-linux"
      "aarch64-linux"
    ];
    forEachSystem = perSystem:
      genAttrs systems (
        system: let
          pkgs = nixpkgs.legacyPackages.${system};
          package = pkgs.callPackage ./packaging/nix/package.nix {};
        in
          perSystem {inherit pkgs system package;}
      );
  in {
    packages = forEachSystem (
      {package, ...}: {
        default = package;
      }
    );
    devShells = forEachSystem (
      {
        pkgs,
        package,
        ...
      }: {
        default = pkgs.mkShell {inputsFrom = [package];};
      }
    );
  };
}
