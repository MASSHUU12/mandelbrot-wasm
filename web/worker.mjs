function parseColor(color) {
  const r = ((color >> (8 * 0)) & 0xff).toString(16).padStart(2, 0);
  const g = ((color >> (8 * 1)) & 0xff).toString(16).padStart(2, 0);
  const b = ((color >> (8 * 2)) & 0xff).toString(16).padStart(2, 0);
  const a = ((color >> (8 * 3)) & 0xff).toString(16).padStart(2, 0);

  return "#" + r + g + b + a;
}

let updateFrameIndex = null;

onmessage = async (event) => {
  const canvas = event.data.canvas;
  const ctx = canvas.getContext("2d");

  const { instance } = await WebAssembly.instantiateStreaming(
    fetch("./main.wasm"),
    {
      env: {
        set_canvas_size: function (w, h) {
          ctx.canvas.width = w;
          ctx.canvas.height = h;
        },
        clear_with_color: function (color) {
          const memory = instance.exports.memory;
          const colorValue = new Uint32Array(memory.buffer, color, 1)[0];

          ctx.fillStyle = parseColor(colorValue);
          ctx.fillRect(0, 0, ctx.canvas.width, ctx.canvas.height);
        },
        fill_rect: function (x, y, w, h, color) {
          const memory = instance.exports.memory;
          const colorValue = new Uint32Array(memory.buffer, color, 1)[0];

          ctx.fillStyle = parseColor(colorValue);
          ctx.fillRect(x, y, w, h);
        },
        fill_circle: function (x, y, radius, color) {
          const memory = instance.exports.memory;
          const colorValue = new Uint32Array(memory.buffer, color, 1)[0];

          ctx.fillStyle = parseColor(colorValue);
          ctx.beginPath();
          ctx.arc(x, y, radius, 0, 2 * Math.PI);
          ctx.fill();
        },
        set_update_frame: function (fIndex) {
          updateFrameIndex = fIndex;
        },
      },
    },
  );

  instance.exports.run();

  const table =
    instance.exports.__indirect_function_table || instance.exports.table;
  const updateFrame = table.get(updateFrameIndex);

  let previousTimestamp;
  function gameLoop(timestamp) {
    const deltaTime = (timestamp - previousTimestamp) * 0.001;
    previousTimestamp = timestamp;

    updateFrame(deltaTime);
    self.requestAnimationFrame(gameLoop);
  }

  self.requestAnimationFrame((timestamp) => {
    previousTimestamp = timestamp;
    self.requestAnimationFrame(gameLoop);
  });
};
