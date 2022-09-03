# GoodBoy
Game Boy Emulator written in C++ & SDL2. Sound is supported too, although the bulk of the work was taken by a PR of another emulator (https://github.com/Dooskington/GameLad/pull/111)

# Emulation Testing
Passes all cpu instruction & instruction timing blargg tests, as well as the PPU acid-2 test.

<img src="https://user-images.githubusercontent.com/10456734/187683951-6408b3f4-741c-4532-af33-c0f426161854.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/187684147-175109e0-aede-44c1-a389-a9d4c855ba94.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/187683612-74ef425f-f152-4234-b281-0402d4dfae00.png" width="250" height="250"> 

# Original Gameboy (DMG) Games Tested
Tested Zelda Link's Awakening, Pokemon Red, Tetris, Speedy Gonzalez, and many other original gameboy ROMs. There is still a VRAM writing but exhibited in Super Mario Land that still needs to be tackled.

<img src="https://user-images.githubusercontent.com/10456734/187683397-a7982db3-10ec-44e9-a168-340bba804979.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/187682634-845a75bb-8d65-4b03-b834-f65a89dab299.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/187682905-b352fc11-6195-4218-abd4-0d74c25f1660.png" width="250" height="250">

# Gameboy Color (CGB) Games Tested
<img src="https://user-images.githubusercontent.com/10456734/188271095-282ff0a7-8138-420b-b11a-148fd0da25aa.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/188271204-d76e20af-c4c7-4d3b-85ed-7f89f5fd4dfd.png" width="250" height="250"> <img src="https://user-images.githubusercontent.com/10456734/188271266-7f3bfe30-eaa0-41b7-9f7f-43604b536afa.png" width="250" height="250">

# Outstanding Work
* Investigate VRAM bug in Super Mario Land
* Implement support for RTC
