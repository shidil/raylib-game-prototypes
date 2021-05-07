const fs = require("fs");

async function readFile(file, format = "utf8") {
  return new Promise((resolve, reject) => {
    fs.readFile(file, format, (err, data) => {
      if (err) {
        reject(err);
        return;
      }

      resolve(data);
    });
  });
}

async function writeFile(file, data) {
  return new Promise((resolve, reject) => {
    fs.writeFile(file, data, (err, data) => {
      if (err) {
        reject(err);
      } else {
        resolve(data);
      }
    });
  });
}

function indexWordsByLength(words) {
  const dict = {};

  for (const word of words) {
    dict[word.length] = dict[word.length] || [];
    dict[word.length].push(word);
  }

  return dict;
}

async function main() {
  const data = await readFile("./word-list.txt");
  const words = data.split("\n");
  const index = indexWordsByLength(words);
  writeFile('./word-data.json', JSON.stringify(index, null ,2));
}

main().then(console.log.bind(console, 'done'));
