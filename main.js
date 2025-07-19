import { $ } from 'bun';

await $`git config --global user.name "GitHub Actions"`;
await $`git config --global user.email "KaKi87@users.noreply.github.com"`;
console.log('Fetching versions');
const items =
    await fetch('https://vivaldi.com/source/')
        .then(response => response.text())
        .then(html => html.matchAll(/<a href="(https:\/\/source\.vivaldi\.com\/vivaldi-source_([0-9.]+)\.tar.xz)".+?modified hidemobile">([^<]+)/g))
        .then(matches => [...matches])
        .then(matches => matches.map(([
            , url
            , version
            , date
        ]) => ({
            url,
            version,
            date
        })))
        .then(items => items.toReversed());
console.log(`Fetched ${items.length} versions`);
for(const {
    url,
    version,
    date
} of items){
    console.log(`Processing version ${version}`);
    if(await $`git tag -l ${version}`.text()){
        console.log(`Version ${version} already exists`);
        continue;
    }
    console.log(`Downloading ${url}`);
    await $`curl -L ${url} | tar xJ`;
    console.log(`Staging version ${version}`);
    await $`git add vivaldi-source`
    console.log(`Committing version ${version}`);
    await $`git commit -m ${version} --date ${date}`;
    console.log(`Tagging version ${version}`);
    await $`git tag -a ${version} -m ${version}`;
    console.log(`Pushing version ${version}`);
    await $`git push`;
    console.log(`Version ${version} pushed`)
}
console.log('Done');