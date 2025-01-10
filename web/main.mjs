const worker = new Worker(new URL("worker.mjs", import.meta.url));
const canvas = document.getElementById("game");
const offscreen = canvas.transferControlToOffscreen();

worker.onmessage = (event) => {
  console.log(`Worker said : ${event.data}`);
};

worker.postMessage({ canvas: offscreen }, [offscreen]);
