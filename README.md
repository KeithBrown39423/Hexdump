<div align="center">
  <br /><br />
  <img src="./assets/banner.png" alt="Banner" />
  <br /><br />
</div>

<div id="user-content-toc" align="center">
  <ul>
    <summary><h1 style="display: inline-block;">Hexdump</h1></summary>
  </ul>

  <p>
    <i>the <b>alternative</b></i> cross platfrom hex dumping utility
    <br />
    <a href="https://github.com/KeithBrown39423/Hexdump/issues"><b>Report Bug</b></a>
    ·
    <a href="https://github.com/KeithBrown39423/Hexdump/issues"><b>Request Feature</b></a>
  </p>
</div>

<p align="center">
  <a href="#key-features">Key Features</a> ·
  <a href="#how-to-use">How To Use</a> ·
  <a href="#credits">Credits</a> ·
  <a href="#license">License</a>
</p>

<div align="center">
  <br /><br />
  <img src="./assets/screenshot.png" alt="Screenshot" />
</div>

## Key Features

* ASCII sidebar
* Colored output
  * Option to remove color
* Output to file
  * With or without color

## How To Use
To use this command, download the binary from the [release](releases/tag/v1.1.0) page.
After downloading, simply move the binary into one of the following folders

### Linux\:
`/usr/bin/`
> Note: This *is* made to replace the standard Hexdump command.
> If you feel uncomfortable replacing it, simply rename the binary to something else.

### Windows:
Although hexdump is compatible with Windows, there is no compiled binary for it.
You will have to compile it yourself using one of the following commands:
```bash
make build
# or
g++ -Wall -Werror -Wpedantic -O3 -I lib -I include src/hexdump.cpp -o bin/hexdump
```
After compiling, move the binary into `C:/Program Files/Hexdump/`
> Note: Don't forget to add `C:/Program Files/Hexdump/` to your PATH.

## Credits
### Libraries
This software uses the following open source library:

* [cxxopts (v3.1.1)](https://github.com/jarro2783/cxxopts/tree/v3.1.1)

### Contributors
Thank you to all the people who have contributed to this project:
<br /><br />
<a href="https://github.com/KeithBrown39423/Hexdump/graphs/contributors">
  <img src="https://contrib.rocks/image?repo=KeithBrown39423/Hexdump"/>
</a>

## License
[MIT](blob/main/LICENSE)
