I will update the `HudLayer.h` file to reflect the new pixel dimensions of the Toolbar.

**Changes:**
1.  **Modify `Classes/HudLayer.h`**:
    - Update `RAW_CELL_WIDTH` to `71.0f`.
    - Update `RAW_CELL_HEIGHT` to `72.0f`.
    - Update `RAW_LEFT_MARGIN` to `23.0f`.
    - Update `RAW_BOTTOM_MARGIN` to `23.0f`.
    - Update `RAW_GAP` to `5.0f`.

2.  **Verify Logic**:
    - The existing `updateInventoryUI` and `onMouseDown` methods in `HudLayer.cpp` already rely on these constants and the `_toolbar` sprite's content size.
    - Once you replace the `Toolbar.png` file in your `Resources` folder, the code will automatically adapt to the new image dimensions (966x113) using these new constants.
    - The `Inventory::TOOLBAR_SIZE` is already set to 12 in `Inventory.h`, so it matches the 12 slots in your new image.

**Next Steps:**
1.  I will apply the code changes to `HudLayer.h`.
2.  After I finish, you should replace the `Toolbar.png` file in your `Resources` folder with the new 966x113 image.
3.  Recompile and run to see the updated toolbar.
