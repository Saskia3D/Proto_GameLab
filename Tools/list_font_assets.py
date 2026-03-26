import pathlib
import unreal


registry = unreal.AssetRegistryHelpers.get_asset_registry()
assets = registry.get_assets_by_path("/Game", recursive=True)

font_like = []
for asset in assets:
    asset_class = str(asset.asset_class_path.asset_name)
    if asset_class in {"Font", "FontFace", "SlateWidgetStyleAsset"}:
        font_like.append((str(asset.package_name), str(asset.asset_name), asset_class))

font_like.sort()

lines = [f"{asset_class}|{package_name}|{asset_name}" for package_name, asset_name, asset_class in font_like]

output_path = pathlib.Path(r"C:\Users\taher\GameLabUbisoft\Proto_GameLab\Tools\font_assets.txt")
output_path.write_text("\n".join(lines), encoding="utf-8")

print(f"WROTE:{output_path}")
