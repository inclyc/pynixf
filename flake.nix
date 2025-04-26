{
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    flake-parts.url = "github:hercules-ci/flake-parts";

    flake-root.url = "github:srid/flake-root";

    treefmt-nix = {
      url = "github:numtide/treefmt-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs =
    {
      nixpkgs,
      flake-parts,
      ...
    }@inputs:
    flake-parts.lib.mkFlake { inherit inputs; } {
      imports = [
        inputs.flake-parts.flakeModules.easyOverlay
        inputs.flake-root.flakeModule
      ];
      perSystem =
        { pkgs, ... }:
        let
          pkg = pkgs.python3Packages.callPackage ./package.nix { };
        in
        {
          packages.default = pkg;
          devShells.default = pkg.overrideAttrs (oldAttrs: {
            nativeBuildInputs = oldAttrs.nativeBuildInputs ++ [ pkgs.uv ];
          });
        };
      systems = nixpkgs.lib.systems.flakeExposed;
    };
}
