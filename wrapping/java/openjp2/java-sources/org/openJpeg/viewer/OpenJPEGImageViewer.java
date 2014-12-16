
/*
 * Copyright (c) 2014, Eric Deas
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS `AS IS'
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 */ 
package org.openJpeg.viewer;

import java.awt.BorderLayout;

public class OpenJPEGImageViewer extends JFrame
{

	private int openWorkerCount = 0;
	private JPanel contentPane;
	private Component clonedJTab;
	private JTabbedPane tabbedPane;
	private JPanel firstPanel;
	private boolean isFirstRemoved = true;
	private boolean addImage;
	private JButton closeButton;

	/**
	 * Launch the application.
	 */
	public static void main(String[] args)
	{
		EventQueue.invokeLater(new Runnable()
		{
			public void run()
			{
				try
				{
					OpenJPEGImageViewer frame = new OpenJPEGImageViewer();
					frame.setVisible(true);
				}
				catch (Exception e)
				{
					e.printStackTrace();
				}
			}
		});
	}

	/**
	 * Create the frame.
	 */
	public OpenJPEGImageViewer()
	{

		setTitle("Open JPEG 2000");
		setDefaultCloseOperation(JFrame.EXIT_ON_CLOSE);
		setBounds(100, 100, 621, 494);
		try
		{
			String osName = System.getProperty("os.name");
			if (osName.contains("Windows"))
				UIManager
						.setLookAndFeel("com.sun.java.swing.plaf.windows.WindowsLookAndFeel");
		}
		catch (ClassNotFoundException | InstantiationException
				| IllegalAccessException | UnsupportedLookAndFeelException e1)
		{
			// TODO Auto-generated catch block
			e1.printStackTrace();
		}
		JMenuBar menuBar = new JMenuBar();
		setJMenuBar(menuBar);

		JMenu mnFile = new JMenu("File");
		menuBar.add(mnFile);

		JMenuItem mntmExit = new JMenuItem("Exit");
		mntmExit.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent arg0)
			{
				System.exit(ABORT);
			}
		});

		contentPane = new JPanel();
		contentPane.setBorder(new EmptyBorder(5, 5, 5, 5));
		contentPane.setLayout(new BorderLayout(0, 0));
		setContentPane(contentPane);

		JPanel panel = new JPanel();
		contentPane.add(panel, BorderLayout.CENTER);

		JTabbedPane ribbonPane = new JTabbedPane(JTabbedPane.TOP);

		final JTabbedPane tabbedPane = new JTabbedPane(JTabbedPane.TOP);
		this.tabbedPane = tabbedPane;
		GroupLayout gl_panel = new GroupLayout(panel);
		gl_panel.setHorizontalGroup(gl_panel
				.createParallelGroup(Alignment.TRAILING)
				.addComponent(ribbonPane)
				.addComponent(tabbedPane, Alignment.LEADING,
						GroupLayout.DEFAULT_SIZE, 595, Short.MAX_VALUE));
		gl_panel.setVerticalGroup(gl_panel.createParallelGroup(
				Alignment.LEADING).addGroup(
				gl_panel.createSequentialGroup()
						.addComponent(ribbonPane, GroupLayout.PREFERRED_SIZE,
								84, GroupLayout.PREFERRED_SIZE)
						.addPreferredGap(ComponentPlacement.RELATED)
						.addComponent(tabbedPane, GroupLayout.DEFAULT_SIZE,
								332, Short.MAX_VALUE)));

		JScrollPane scrollPane = new JScrollPane();
		tabbedPane.addTab("Empty View", null, scrollPane, null);

		final JPanel panel_1 = new JPanel();
		this.firstPanel = panel_1;
		scrollPane.setViewportView(panel_1);

		JPanel generalTab = new JPanel();
		ribbonPane.addTab("General", null, generalTab, null);

		JButton openButton = new JButton("Open");
		openButton.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent arg0)
			{
				Thread t = new Thread(new Runnable()
				{
					@Override
					public void run()
					{

						// TODO Auto-generated method stub
						JFileChooser fileChooser = new JFileChooser();
						fileChooser.setCurrentDirectory(new File(System
								.getProperty("user.home")));
						int result = fileChooser
								.showOpenDialog(OpenJPEGImageViewer.this);

						File selectedFile = null;
						if (result == JFileChooser.APPROVE_OPTION)
						{
							selectedFile = fileChooser.getSelectedFile();
						}

						if (selectedFile != null)
						{
							try
							{
								final byte[] bArray = getBytesFromFile(selectedFile);

								OpenJPEGJavaDecoder dec = new OpenJPEGJavaDecoder();

								dec.setCompressedStream(bArray);

								if (tabbedPane.getTabCount() == 1
										&& isFirstRemoved)
								{
									synchronized (OpenJPEGImageViewer.this)
									{
										tabbedPane.setTitleAt(0,
												"Loading Image...");
										isFirstRemoved = false;
										openWorkerCount++;
										repaint();
									}

									dec.decodeJ2KtoImage();
									final BufferedImage img = read(dec);
									final JLabel label = new JLabel(
											new ImageIcon(img));
									label.setPreferredSize(new Dimension(img
											.getWidth(), img.getHeight()));
									firstPanel.add(label);
									firstPanel.setPreferredSize(new Dimension(
											img.getWidth(), img.getHeight()));
									tabbedPane.setTitleAt(0,
											selectedFile.getName());
									synchronized(OpenJPEGImageViewer.this)
									{
										openWorkerCount--;
										repaint();
									}
								}
								else
								{
									OpenJPEGImageViewer.this.addTab(
											selectedFile, dec);
								}
							}
							catch (IOException e)
							{
								// TODO Auto-generated catch block
								e.printStackTrace();
							}
						}
					}
				});
				t.start();

			}
		});

		JMenuItem mntmOpen = new JMenuItem("Open");
		mntmOpen.addActionListener(new ActionListener()
		{

			@Override
			public void actionPerformed(ActionEvent e)
			{
				// TODO Auto-generated method stub

			}

		}

		);
		mnFile.add(mntmOpen);
		mnFile.add(mntmExit);

		JButton closeButton = new JButton("Close Image");
		this.closeButton = closeButton;
		closeButton.addActionListener(new ActionListener()
		{
			public void actionPerformed(ActionEvent e)
			{
				removeTab();
			}
		});
		GroupLayout gl_generalTab = new GroupLayout(generalTab);
		gl_generalTab.setHorizontalGroup(gl_generalTab.createParallelGroup(
				Alignment.LEADING).addGroup(
				gl_generalTab
						.createSequentialGroup()
						.addContainerGap()
						.addComponent(openButton, GroupLayout.PREFERRED_SIZE,
								85, GroupLayout.PREFERRED_SIZE)
						.addPreferredGap(ComponentPlacement.RELATED)
						.addComponent(closeButton)
						.addContainerGap(400, Short.MAX_VALUE)));
		gl_generalTab.setVerticalGroup(gl_generalTab.createParallelGroup(
				Alignment.LEADING).addGroup(
				gl_generalTab
						.createSequentialGroup()
						.addContainerGap()
						.addGroup(
								gl_generalTab
										.createParallelGroup(
												Alignment.TRAILING, false)
										.addComponent(closeButton,
												Alignment.LEADING,
												GroupLayout.DEFAULT_SIZE,
												GroupLayout.DEFAULT_SIZE,
												Short.MAX_VALUE)
										.addComponent(openButton,
												Alignment.LEADING,
												GroupLayout.DEFAULT_SIZE, 35,
												Short.MAX_VALUE))
						.addContainerGap(GroupLayout.DEFAULT_SIZE,
								Short.MAX_VALUE)));
		generalTab.setLayout(gl_generalTab);
		panel.setLayout(gl_panel);
	}

	private File openFile()
	{
		JFileChooser fileChooser = new JFileChooser();
		fileChooser.setCurrentDirectory(new File(System
				.getProperty("user.home")));
		int result = fileChooser.showOpenDialog(this);

		File selectedFile = null;
		if (result == JFileChooser.APPROVE_OPTION)
		{

			selectedFile = fileChooser.getSelectedFile();
		}

		return selectedFile;

	}
	
	public void paint(Graphics g)
	{
		super.paint(g);
		if(this.openWorkerCount != 0)
		{
			this.closeButton.setEnabled(false);
		}
		else
		{
			this.closeButton.setEnabled(true);
		}
	}

	private static byte[] getBytesFromFile(File file) throws IOException
	{
		long length = file.length();
		if (length > Integer.MAX_VALUE)
		{
			throw new IOException("File is too large!");
		}
		byte[] bytes = new byte[(int) length];
		int offset = 0;
		int numRead = 0;
		InputStream is = new FileInputStream(file);
		try
		{
			while (offset < bytes.length
					&& (numRead = is.read(bytes, offset, bytes.length - offset)) >= 0)
			{
				offset += numRead;
			}
		}
		finally
		{
			is.close();
		}
		if (offset < bytes.length)
		{
			throw new IOException("Could not completely read file "
					+ file.getName());
		}
		return bytes;
	}

	public static BufferedImage read(OpenJPEGJavaDecoder decoder)
			throws IOException
	{
		// checkImageIndex(imageIndex);
		if (decoder == null)
			return null;
		int width = decoder.getWidth();
		int height = decoder.getHeight();
		BufferedImage bufimg = null;
		byte[] buf8;
		short[] buf16;
		int[] buf24;
		if ((buf24 = decoder.getImage24()) != null)
		{
			int[] bitMasks = new int[] { 0xFF0000, 0xFF00, 0xFF, 0xFF000000 };
			SinglePixelPackedSampleModel sm = new SinglePixelPackedSampleModel(
					DataBuffer.TYPE_INT, width, height, bitMasks);
			DataBufferInt db = new DataBufferInt(buf24, buf24.length);
			WritableRaster wr = Raster
					.createWritableRaster(sm, db, new Point());

			bufimg = new BufferedImage(ColorModel.getRGBdefault(), wr, false,
					null);
		}
		else if ((buf16 = decoder.getImage16()) != null)
		{
			int[] bits = { 16 };
			ColorModel cm = new ComponentColorModel(
					ColorSpace.getInstance(ColorSpace.CS_GRAY), bits, false,
					false, Transparency.OPAQUE, DataBuffer.TYPE_USHORT);
			SampleModel sm = cm.createCompatibleSampleModel(width, height);
			DataBufferUShort db = new DataBufferUShort(buf16, width * height
					* 2);
			WritableRaster ras = Raster.createWritableRaster(sm, db, null);
			bufimg = new BufferedImage(cm, ras, false, null);
		}
		else if ((buf8 = decoder.getImage8()) != null)
		{
			int[] bits = { 8 };
			ColorModel cm = new ComponentColorModel(
					ColorSpace.getInstance(ColorSpace.CS_GRAY), bits, false,
					false, Transparency.OPAQUE, DataBuffer.TYPE_BYTE);
			SampleModel sm = cm.createCompatibleSampleModel(width, height);
			DataBufferByte db = new DataBufferByte(buf8, width * height);
			WritableRaster ras = Raster.createWritableRaster(sm, db, null);
			bufimg = new BufferedImage(cm, ras, false, null);
		}
		return bufimg;
	}

	private void addTab(File f, OpenJPEGJavaDecoder decoder)
	{
		JScrollPane scrollPane = new JScrollPane();
		int workerID = 0;
		JPanel panel_1 = null;
		synchronized (this)
		{
			workerID = tabbedPane.getTabCount();
			openWorkerCount++;
			panel_1 = new JPanel();
			scrollPane.setViewportView(panel_1);
			this.tabbedPane.addTab("Loading Image", scrollPane);
			repaint();
			
		}
		
		
		decoder.decodeJ2KtoImage();
		BufferedImage img = null;
		try
		{
			img = read(decoder);
		}
		catch (IOException e)
		{
			// TODO Auto-generated catch block
			e.printStackTrace();
		}
		if (img != null)
		{
			final JLabel label = new JLabel(new ImageIcon(img));
			label.setPreferredSize(new Dimension(img.getWidth(), img
					.getHeight()));
			panel_1.add(label);
			panel_1.setPreferredSize(new Dimension(img.getWidth(), img
					.getHeight()));
			this.tabbedPane.setTitleAt(workerID, f.getName());
		}
		synchronized (this)
		{
			openWorkerCount--;
			repaint();
		}
	}

	private void removeTab()
	{
		if (this.tabbedPane.getTabCount() == 1 && this.openWorkerCount == 0)
		{
			if (!isFirstRemoved)
			{
				this.firstPanel = (JPanel) ((JScrollPane) this.tabbedPane
						.getSelectedComponent()).getViewport().getComponent(0);
				this.firstPanel.removeAll();
				tabbedPane.setTitleAt(0, "No image");
				tabbedPane.paintComponents(tabbedPane.getGraphics());
				isFirstRemoved = true;

			}
		}
		else if(this.openWorkerCount == 0)
		{
			this.tabbedPane.removeTabAt(tabbedPane.getSelectedIndex());
			tabbedPane.paintComponents(tabbedPane.getGraphics());
		}
	}
}
